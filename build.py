#!/usr/bin/env python3
"""Dedicated D125/D133 USB display firmware builder; no SDK build system."""
import argparse
import concurrent.futures
import hashlib
import json
import os
import pathlib
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent
sys.dont_write_bytecode = True
TOOLS = ROOT / 'tools/scripts'
GCC = ROOT / 'toolchain/bin'
PREFIX = 'riscv64-unknown-elf-'
os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
os.environ['PATH'] = str(GCC) + os.pathsep + str(TOOLS) + os.pathsep + os.environ['PATH']
sys.path.insert(0, str(TOOLS))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(args, log=None, cwd=ROOT):
    result = subprocess.run([str(a) for a in args], cwd=cwd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding='utf-8', errors='replace')
    if log:
        log.parent.mkdir(parents=True, exist_ok=True)
        log.write_text(result.stdout, encoding='utf-8')
    if result.returncode:
        raise RuntimeError('Command failed: {}\n{}'.format(' '.join(map(str, args)), result.stdout[-6000:]))
    return result.stdout


def helper(name, *args):
    return run([sys.executable, TOOLS / name] + list(args))


def safe_remove(path):
    path = path.resolve()
    path.relative_to(ROOT / 'build')
    if path.is_symlink():
        raise ValueError('Refusing a symlink: ' + str(path))
    shutil.rmtree(path)


def configure(graph_name, variant, out):
    config = out / 'config'
    config.mkdir(parents=True, exist_ok=True)
    header = (ROOT / 'firmware/configs' / (graph_name + '.h')).read_text()
    write_if_changed(config / 'rtconfig.h', header)
    write_if_changed(config / 'cconfig.h', (ROOT / 'cconfig.h').read_text())
    defines = ['#ifndef FIRMWARE_VARIANT_H', '#define FIRMWARE_VARIANT_H']
    for key, value in variant['timing'].items():
        defines.append('#define FIRMWARE_{} {}'.format(key.upper(), value))
    defines += ['#define FIRMWARE_JPEG_QUALITY {}'.format(variant['jpeg_quality']),
                '#define FIRMWARE_MAX_FPS {}'.format(variant['fps']), '#endif']
    write_if_changed(config / 'variant.h', '\n'.join(defines) + '\n')
    return config


def write_if_changed(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


def dependencies(path):
    if not path.exists():
        return []
    text = path.read_text().replace('\\\n', '')
    text = text.split(': ', 1)[-1]
    return [pathlib.Path(p) for p in re.findall(r'[^\s]+', text)]


def build_elf(graph_name, variant, jobs, output_name=None):
    graph = json.loads((ROOT / 'firmware/graphs' / (graph_name + '.json')).read_text())
    out = ROOT / 'build' / (output_name or graph_name)
    out.mkdir(parents=True, exist_ok=True)
    config = configure(graph_name, variant, out)
    chip = variant['chip']
    image_cfg = ROOT / 'target' / chip / 'display-nor/pack/image_cfg.json'
    helper('gen_partition_table.py', '-c', image_cfg, '-o', config / 'partition_table.h',
           '-j', out / 'partition.json')
    helper('gen_partition_file_list.py', '-c', image_cfg, '-o', out / 'partition_file_list.h')

    def expand(text):
        return text.replace('${ROOT}', ROOT.as_posix())
    includes = ['-I' + str(config)] + ['-I' + expand(i) for i in graph['includes']]
    defines = ['-D' + d for d in graph['defines']]
    forced = ['-include', str(config / 'variant.h')]
    linker = re.search(r'-T (.+)$', expand(graph['linkflags'])).group(1)
    linker_source = pathlib.Path(linker + '.S')
    linker_output = out / 'firmware.ld'
    run([GCC / (PREFIX + 'gcc.exe'), '-E', '-P', '-x', 'c', '-I' + str(config),
         str(linker_source), '-o', linker_output])

    def compile_one(entry):
        source = pathlib.Path(expand(entry['source']))
        relative = source.relative_to(ROOT)
        obj = out / 'objects' / (relative.as_posix() + '.o')
        obj.parent.mkdir(parents=True, exist_ok=True)
        compiler = 'g++' if source.suffix == '.cpp' else 'gcc'
        flags = graph['asflags'] if source.suffix == '.S' else graph['cxxflags'] if compiler == 'g++' else graph['cflags']
        flags = shlex.split(expand(flags)) + shlex.split(entry['cflags'])
        local = ['-I' + expand(p) for p in entry['cpppath']] + ['-D' + d for d in entry['defines']]
        command = [GCC / (PREFIX + compiler + '.exe')] + flags + includes + defines + local + forced
        command += ['-MMD', '-MF', str(obj.with_suffix('.d')), '-c', source, '-o', obj]
        signature = hashlib.sha256(' '.join(map(str, command)).encode()).hexdigest()
        sigfile = obj.with_suffix('.sig')
        # Each selected variant owns an output directory; config changes also
        # invalidate its cached objects. Header timestamps are checked below.
        stamp = max(source.stat().st_mtime, (config / 'rtconfig.h').stat().st_mtime,
                    (config / 'variant.h').stat().st_mtime)
        deps = dependencies(obj.with_suffix('.d'))
        stale = not deps or any(not p.exists() or p.stat().st_mtime > obj.stat().st_mtime for p in deps) if obj.exists() else True
        if not obj.exists() or not sigfile.exists() or sigfile.read_text() != signature or obj.stat().st_mtime < stamp or stale:
            run(command, obj.with_suffix('.log'))
            sigfile.write_text(signature)
        return obj

    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        objects = list(pool.map(compile_one, graph['sources']))
    elf = out / (chip + '.elf')
    flags = re.sub(r'-Wl,-Map=\S+', '-Wl,-Map=' + str(out / (chip + '.map')).replace('\\', '/'), expand(graph['linkflags']))
    flags = flags[:flags.rfind(' -T ')]
    args = shlex.split(flags) + ['-T', str(linker_output)] + [str(o) for o in objects]
    args += ['-L' + expand(p) for p in graph['libpaths']]
    args += ['-Wl,--start-group']
    args += ['-l' + lib for lib in graph['libs'] if lib in ('c', 'm')]
    args += ['-Wl,--whole-archive'] + ['-l' + lib for lib in graph['libs'] if lib not in ('c', 'm')]
    args += ['-Wl,--no-whole-archive', '-Wl,--end-group', '-o', str(elf)]
    rsp = out / 'link.rsp'
    rsp.write_text(' '.join('"' + a.replace('\\', '/') + '"' for a in args))
    run([GCC / (PREFIX + 'gcc.exe'), '@' + str(rsp)], out / 'link.log')
    run([GCC / (PREFIX + 'objcopy.exe'), '-O', 'binary', elf, out / (chip + '.bin')])
    print(run([GCC / (PREFIX + 'size.exe'), elf]).strip(), flush=True)
    return out, config


def pack(variant_id, variant, out, config, boot_out):
    chip = variant['chip']
    pack_dir = ROOT / 'target' / chip / 'display-nor/pack'
    cfg = json.loads((pack_dir / 'image_cfg.json').read_text())
    header = (boot_out / 'config/rtconfig.h').read_text()
    match = re.search(r'^#define AIC_BOOTLOADER_TEXT_BASE\s+(.+)$', header, re.M)
    text_base = int(match.group(1).strip('() '), 0)
    text = json.dumps(cfg).replace('CONFIG_AIC_BOOTLOADER_TEXT_BASE', hex(text_base))
    text = text.replace('CONFIG_AIC_BOOTLOADER_LOAD_BASE', hex(text_base - 0x100))
    cfg = json.loads(text)
    cfg['image']['info']['product'] = variant['product']
    cfg['image']['info']['version'] = variant.get('version', '1.0.0')
    (out / 'image_cfg.json').write_text(json.dumps(cfg, indent=2))
    shutil.copy2(boot_out / (chip + '.bin'), out / 'bootloader.bin')
    shutil.copy2(pack_dir / 'env.txt', out / 'env.txt')
    pbp_cfg = pack_dir / 'pbp_cfg.json'
    if not pbp_cfg.exists():
        pbp_cfg = ROOT / 'bsp/artinchip/sys' / chip / 'pbp_cfg.json'
    shutil.copy2(pbp_cfg, out / 'pbp_cfg.json')
    for file in (ROOT / 'bsp/artinchip/sys' / chip).glob('*.pbp'):
        shutil.copy2(file, out / file.name)
    helper('makefatfs.py', '--fullpart', '--volab', 'default', '--cluster', '8', '--sector', '512',
           '--tooldir', str(TOOLS) + '/', '--inputdir', 'application/rt-thread/usbdisp/assets',
           '--outfile', out / 'rodata.fatfs', '--reserved', '50')
    helper('mk_private_resource.py', '-l', str(out / 'pbp_cfg.json') + ',' + str(out / 'partition.json'),
           '-o', out / 'pbp_cfg.bin')
    if chip == 'd12p':
        helper('mk_prebootprog.py', '-m', 'PDAT', '-i', out / 'pbp_cfg.bin', '-o', out / 'pbp_cfg.bin')
    from elf2image import elf2its
    elf2its(str(out / (chip + '.elf')), cfg['image']['info']['version'], str(out / (chip + '_os.its')))
    helper('mk_image.py', '-c', out / 'image_cfg.json', '-d', str(out) + '/')
    images = list(out.glob('*.img'))
    if len(images) != 1:
        raise RuntimeError('Expected one IMG for ' + variant_id)
    verify_image(images[0], out, variant)
    dest = ROOT / 'images' / (variant_id + '.img')
    dest.parent.mkdir(exist_ok=True)
    shutil.copy2(images[0], dest)
    return dict(variant=variant_id, path='images/' + dest.name, bytes=dest.stat().st_size,
                sha256=sha(dest), width=variant['timing']['width'], height=variant['timing']['height'])


def verify_image(image_path, out, variant):
    image = image_path.read_bytes()
    meta_offset, meta_size = struct.unpack_from('<II', image, 332)
    components = {}
    for pos in range(meta_offset, meta_offset + meta_size, 512):
        name = image[pos+8:pos+72].split(b'\0')[0].decode()
        offset, size = struct.unpack_from('<II', image, pos + 136)
        if name:
            if offset + size > len(image):
                raise ValueError('Invalid image extent')
            components[name] = image[offset:offset+size]
    for name, file in [('image.target.rodata', 'rodata.fatfs'), ('image.target.spl', 'bootloader.aic')]:
        if components[name] != (out / file).read_bytes():
            raise ValueError('Image component mismatch: ' + name)
    with tempfile.TemporaryDirectory(prefix='udisp-check-') as temp:
        run([TOOLS / 'mcopy.exe', '-i', out / 'rodata.fatfs', '::/video/video.bin', 'video.bin'], cwd=temp)
        if sha(pathlib.Path(temp) / 'video.bin') != sha(ROOT / 'application/rt-thread/usbdisp/assets/video/video.bin'):
            raise ValueError('Packaged idle video mismatch')
    cfg = json.loads((out / 'image_cfg.json').read_text())['spi-nor']
    sizes = {'k': 1024, 'm': 1048576}
    total = sum(int(p['size'][:-1]) * sizes[p['size'][-1]] for p in cfg['partitions'].values())
    if cfg['size'] != '32m' or total > 32 * 1048576:
        raise ValueError('NOR partitions exceed 32 MiB')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('variants', nargs='*', help='Variant IDs; omitted means all six')
    parser.add_argument('--list', action='store_true')
    parser.add_argument('--jobs', type=int, default=min(os.cpu_count() or 4, 8))
    parser.add_argument('--keep-build', action='store_true')
    parser.add_argument('--clean', action='store_true', help='Remove generated build and images directories')
    args = parser.parse_args()
    variants = json.loads((ROOT / 'variants.json').read_text())
    if args.list:
        print('\n'.join(variants))
        return
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    if ' ' in str(ROOT):
        parser.error('Use a directory without spaces for the bundled FAT tools')
    if args.clean:
        if (ROOT / 'build/.lock').exists():
            parser.error('A build is active; cannot clean')
        for name in ('build', 'images'):
            directory = ROOT / name
            if directory.exists():
                if directory.is_symlink() or directory.resolve().parent != ROOT:
                    raise ValueError('Unexpected output path: ' + str(directory))
                shutil.rmtree(directory)
        print('Generated files removed.')
        return
    selected = args.variants or list(variants)
    if any(v not in variants for v in selected):
        parser.error('Unknown variant; use --list')
    for variant_id in selected:
        variant = variants[variant_id]
        if not re.fullmatch(r'[a-z0-9][a-z0-9-]*', variant_id):
            parser.error('Variant IDs must use lowercase letters, digits and hyphens')
        if variant['chip'] not in ('d12p', 'd13x'):
            parser.error('Only D125/d12p and D133/d13x are supported')
        for key in ('width', 'height'):
            value = variant['timing'][key]
            if not isinstance(value, int) or value < 1 or value > 4096:
                parser.error('Invalid ' + key + ' for ' + variant_id)
        if variant['timing']['width'] % 2 or variant['timing']['height'] % 2:
            parser.error('NV12 requires even width and height')
        if not re.fullmatch(r'[a-z0-9_-]+', variant['product']):
            parser.error('Invalid image product name')
        if not re.fullmatch(r'[0-9]+\.[0-9]+\.[0-9]+', variant.get('version', '1.0.0')):
            parser.error('Use a numeric major.minor.patch version')
    (ROOT / 'build').mkdir(exist_ok=True)
    lock = ROOT / 'build/.lock'
    try:
        handle = lock.open('x')
    except FileExistsError:
        parser.error('Another build is active, or build/.lock remains after interruption')
    try:
        bootloaders = {}
        results = []
        for variant_id in selected:
            variant = variants[variant_id]
            chip = variant['chip']
            if chip not in bootloaders:
                print('BOOT ' + chip, flush=True)
                boot_variant = next(v for v in variants.values() if v['chip'] == chip and 'txw935003b' in v['graph'])
                bootloaders[chip], _ = build_elf(chip + '_display-nor_baremetal_bootloader', boot_variant, args.jobs)
            print('BUILD ' + variant_id, flush=True)
            out, config = build_elf(variant['graph'], variant, args.jobs, variant_id)
            results.append(pack(variant_id, variant, out, config, bootloaders[chip]))
            print('OK ' + variant_id, flush=True)
        (ROOT / 'images/manifest.json').write_text(json.dumps(results, indent=2) + '\n')
        print('All selected images verified.', flush=True)
        if not args.keep_build:
            for directory in list((ROOT / 'build').iterdir()):
                if directory.is_dir():
                    safe_remove(directory)
    finally:
        handle.close()
        lock.unlink()


if __name__ == '__main__':
    try:
        main()
    except Exception as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
