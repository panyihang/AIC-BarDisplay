# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
# Pure Python ELF firmware image builder — no external readelf / objcopy.
# Only uses Python 3.7+ standard library.
"""
elf2image — internal implementation.

Public API (re-exported from ``__init__.py``):
    elf2its()  — full pipeline: parse ELF → extract binaries → generate .its
    main()     — CLI entry point
"""
import argparse
import glob
import os
import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional

# ── ELF constants ──────────────────────────────────────────────────
_ELFMAG = b'\x7fELF'
_ELFCLASS32 = 1
_ELFCLASS64 = 2
_ELFDATA2LSB = 1
_ELFDATA2MSB = 2
_PT_LOAD = 1
_SHT_NOBITS = 8
_SHF_ALLOC = 0x2


# ── Internal data models ───────────────────────────────────────────
@dataclass
class _Phdr:
    p_type: int = 0
    p_offset: int = 0
    p_vaddr: int = 0
    p_filesz: int = 0
    p_memsz: int = 0
    p_flags: int = 0


@dataclass
class _Shdr:
    name: str = ""
    sh_type: int = 0
    sh_flags: int = 0
    sh_addr: int = 0
    sh_offset: int = 0
    sh_size: int = 0


@dataclass
class _ElfFile:
    ei_class: int = 0
    endian: str = '<'
    entry_point: int = 0
    phdrs: List[_Phdr] = field(default_factory=list)
    shdrs: List[_Shdr] = field(default_factory=list)
    raw: bytes = b''


@dataclass
class _Segment:
    load_addr: int = 0
    entry_point: int = 0
    sections: List[_Shdr] = field(default_factory=list)


@dataclass
class _FirmwareImage:
    entry_point: int = 0
    segments: List[_Segment] = field(default_factory=list)


# ── ELF parsing ────────────────────────────────────────────────────
def _parse_elf(raw: bytes) -> _ElfFile:
    """Parse raw ELF bytes into an _ElfFile structure."""
    if raw[:4] != _ELFMAG:
        raise ValueError("Not a valid ELF file")

    elf = _ElfFile()
    elf.raw = raw
    elf.ei_class = raw[4]
    elf.endian = '<' if raw[5] == _ELFDATA2LSB else '>'
    e = elf.endian

    if elf.ei_class == _ELFCLASS32:
        (e_type, e_machine, e_version,
         elf.entry_point, e_phoff, e_shoff,
         e_flags, e_ehsize,
         e_phentsize, e_phnum,
         e_shentsize, e_shnum, e_shstrndx) = struct.unpack_from(
            e + 'HHIIIIIHHHHHH', raw, 16)
    elif elf.ei_class == _ELFCLASS64:
        (e_type, e_machine, e_version,
         elf.entry_point, e_phoff, e_shoff,
         e_flags, e_ehsize,
         e_phentsize, e_phnum,
         e_shentsize, e_shnum, e_shstrndx) = struct.unpack_from(
            e + 'HHIQQQIHHHHHH', raw, 16)
    else:
        raise ValueError(f"Unsupported ELF class: {elf.ei_class}")

    # Parse program headers
    for i in range(e_phnum):
        off = e_phoff + i * e_phentsize
        if elf.ei_class == _ELFCLASS32:
            (p_type, p_offset, p_vaddr, p_paddr,
             p_filesz, p_memsz, p_flags, p_align) = struct.unpack_from(
                e + 'IIIIIIII', raw, off)
        else:
            (p_type, p_flags, p_offset, p_vaddr, p_paddr,
             p_filesz, p_memsz, p_align) = struct.unpack_from(
                e + 'IIQQQQQQ', raw, off)
        elf.phdrs.append(_Phdr(p_type, p_offset, p_vaddr, p_filesz, p_memsz, p_flags))

    # Parse section headers
    shdrs_raw = []
    for i in range(e_shnum):
        off = e_shoff + i * e_shentsize
        if elf.ei_class == _ELFCLASS32:
            (sh_name, sh_type, sh_flags, sh_addr,
             sh_offset, sh_size, sh_link, sh_info,
             sh_addralign, sh_entsize) = struct.unpack_from(
                e + 'IIIIIIIIII', raw, off)
        else:
            (sh_name, sh_type, sh_flags, sh_addr,
             sh_offset, sh_size, sh_link, sh_info,
             sh_addralign, sh_entsize) = struct.unpack_from(
                e + 'IIQQQQIIQQ', raw, off)
        sh = _Shdr(sh_type=sh_type, sh_flags=sh_flags,
                   sh_addr=sh_addr, sh_offset=sh_offset, sh_size=sh_size)
        elf.shdrs.append(sh)
        shdrs_raw.append(sh_name)  # name offset, resolved below

    # Resolve section names from string table
    if e_shstrndx < len(elf.shdrs):
        strtab = elf.shdrs[e_shstrndx]
        strtab_data = raw[strtab.sh_offset:strtab.sh_offset + strtab.sh_size]
        for sh, name_off in zip(elf.shdrs, shdrs_raw):
            if name_off < len(strtab_data):
                if b'\x00' in strtab_data[name_off:]:
                    end = strtab_data.index(b'\x00', name_off)
                else:
                    end = len(strtab_data)
                sh.name = strtab_data[name_off:end].decode('ascii', errors='replace')

    return elf


# ── Firmware image builder ─────────────────────────────────────────
def _build_firmware_image(elf: _ElfFile) -> _FirmwareImage:
    """Build _FirmwareImage from ELF: extract LOAD segments, map sections, XIP swap."""
    # Filter LOAD segments with file content
    load_phdrs = [ph for ph in elf.phdrs if ph.p_type == _PT_LOAD and ph.p_filesz > 0]

    # Map sections to each LOAD segment
    segments = []
    for ph in load_phdrs:
        secs = sorted(
            [sh for sh in elf.shdrs
             if (sh.sh_flags & _SHF_ALLOC) and sh.sh_size > 0
             and ph.p_vaddr <= sh.sh_addr < ph.p_vaddr + ph.p_memsz],
            key=lambda s: s.sh_addr
        )
        segments.append(_Segment(
            load_addr=ph.p_vaddr,
            entry_point=elf.entry_point,
            sections=secs,
        ))

    # XIP swap: entry code must be in segment 0
    if elf.entry_point > 0x60000000:
        for i, seg in enumerate(segments):
            if seg.load_addr == elf.entry_point and i:
                segments[0], segments[i] = segments[i], segments[0]

    return _FirmwareImage(entry_point=elf.entry_point, segments=segments)


# ── Binary extraction (replaces objcopy -O binary -j ...) ─────────
def _extract_segment_binary(raw: bytes, seg: _Segment) -> bytes:
    """Extract segment binary — equivalent to objcopy -O binary -j .sec1 -j .sec2 ...

    Verified against GNU objcopy 10.2:
      - Only PROGBITS sections (sh_type != SHT_NOBITS) produce file data
      - NOBITS sections (.bss etc.) are skipped entirely
      - Output spans [lowest .. highest addr+size] of PROGBITS, gaps zeroed
    """
    progbits = sorted(
        [s for s in seg.sections if s.sh_type != _SHT_NOBITS],
        key=lambda s: s.sh_addr
    )
    if not progbits:
        return b''

    base = progbits[0].sh_addr
    end = progbits[-1].sh_addr + progbits[-1].sh_size
    buf = bytearray(end - base)

    for sh in progbits:
        off = sh.sh_addr - base
        buf[off:off + sh.sh_size] = raw[sh.sh_offset:sh.sh_offset + sh.sh_size]

    return bytes(buf)


def _extract_all_binaries(raw: bytes, image: _FirmwareImage, out_dir: str) -> None:
    """Clean old seg*.bin and write all segment binaries."""
    for old in glob.glob(os.path.join(out_dir, 'seg*.bin')):
        os.remove(old)

    for i, seg in enumerate(image.segments):
        data = _extract_segment_binary(raw, seg)
        with open(os.path.join(out_dir, f'seg{i}.bin'), 'wb') as f:
            f.write(data)


# ── ITS generation ─────────────────────────────────────────────────
def _generate_its(image: _FirmwareImage, version: str, output_path: str, *,
                  description: str = "Luban-lite firmware",
                  hash_algos: Optional[List[str]] = None,
                  signature: Optional[Dict[str, str]] = None,
                  encryption: Optional[Dict[str, str]] = None) -> None:
    """Generate .its (Image Tree Source) file from _FirmwareImage.

    Args:
        image:       Firmware image description.
        version:     Version string written to the ITS header.
        output_path: Output .its file path.
        description: Firmware description (default: "Luban-lite firmware").
        hash_algos:  List of hash algorithms for each segment node.
                     Default ["crc32", "md5"]. Pass [] to omit hash nodes.
        signature:   Optional signature config dict, e.g.
                     {"algo": "sha256", "key-name-hint": "app_key"}.
        encryption:  Optional encryption config dict, e.g.
                     {"algo": "aes128", "key-name-hint": "app_aes",
                      "iv-name-hint": "app_iv"}.
    """
    if hash_algos is None:
        hash_algos = ["crc32", "md5"]

    n = len(image.segments)
    lines = []

    # Header
    lines.append('/dts-v1/;')
    lines.append('')
    lines.append('/ {')
    lines.append('\tdescription = "ArtInChip Luban-lite";')
    lines.append('\t#address-cells = <1>;')
    lines.append('')
    lines.append('\timages {')
    lines.append(f'\t\tversion = "{version}";')

    # Segment nodes (2-tab indent for node name, matching original format)
    for i, seg in enumerate(image.segments):
        lines.append(f'\t\tseg{i} {{')
        lines.append(f'\t\t\tdescription = "ArtInChip segment {i}";')
        lines.append(f'\t\t\tdata = /incbin/("./seg{i}.bin");')
        lines.append(f'\t\t\ttype = "firmware";')
        lines.append(f'\t\t\tarch = "riscv";')
        lines.append(f'\t\t\tos = "artinchip";')
        lines.append(f'\t\t\tload = <{hex(seg.load_addr)}>;')
        lines.append(f'\t\t\tentry = <{hex(seg.entry_point)}>;')
        for j, algo in enumerate(hash_algos):
            lines.append(f'\t\t\thash-{j + 1} {{')
            lines.append(f'\t\t\t\talgo = "{algo}";')
            lines.append(f'\t\t\t}};')
        if signature:
            lines.append(f'\t\t\tsignature-1 {{')
            lines.append(f'\t\t\t\talgo = "{signature["algo"]}";')
            lines.append(f'\t\t\t\tkey-name-hint = "{signature["key-name-hint"]}";')
            lines.append(f'\t\t\t}};')
        if encryption:
            lines.append(f'\t\t\tcipher {{')
            lines.append(f'\t\t\t\talgo = "{encryption["algo"]}";')
            if "key-name-hint" in encryption:
                lines.append(f'\t\t\t\tkey-name-hint = "{encryption["key-name-hint"]}";')
            if "iv-name-hint" in encryption:
                lines.append(f'\t\t\t\tiv-name-hint = "{encryption["iv-name-hint"]}";')
            lines.append(f'\t\t\t}};')
        lines.append(f'\t\t}};')

    # Tail — note: original format has `};` at 1-tab and `};` at 0-tab
    lines.append('\t};')
    lines.append('')
    lines.append('\tconfigurations {')
    lines.append('\t\tdefault = "conf-1";')
    lines.append('\t\tconf-1 {')
    lines.append(f'\t\t\tdescription = "{description}";')
    fw_list = ', '.join(f'"seg{i}"' for i in range(n))
    lines.append(f'\t\t\tfirmware = {fw_list};')
    lines.append('\t\t};')
    lines.append('\t};')
    lines.append('};')
    lines.append('')  # trailing newline

    with open(output_path, 'w') as f:
        f.write('\n'.join(lines))


# ── Public API ─────────────────────────────────────────────────────
def elf2its(elf_file: str, version: str, its_file: str, *,
            description: str = "Luban-lite firmware",
            hash_algos: Optional[List[str]] = None,
            signature: Optional[Dict[str, str]] = None,
            encryption: Optional[Dict[str, str]] = None) -> _FirmwareImage:
    """Full pipeline: parse ELF → extract binaries → generate ITS.

    Args:
        elf_file:    Path to the input ELF file.
        version:     Version string written into the ITS header.
        its_file:    Output path for the .its file (seg*.bin go in the same directory).
        description: Firmware description text.
        hash_algos:  Hash algorithms per segment (default ["crc32", "md5"]).
        signature:   Optional signature config, e.g. {"algo": "sha256", "key-name-hint": "app_key"}.
        encryption:  Optional encryption config, e.g. {"algo": "aes-128-cbc", ...}.

    Returns:
        The parsed _FirmwareImage for further inspection if needed.
    """
    out_dir = os.path.dirname(os.path.abspath(its_file))
    with open(elf_file, 'rb') as f:
        raw = f.read()

    elf = _parse_elf(raw)
    image = _build_firmware_image(elf)
    _extract_all_binaries(raw, image, out_dir)
    _generate_its(image, version, its_file,
                  description=description, hash_algos=hash_algos,
                  signature=signature, encryption=encryption)
    return image


# ── CLI entry point ────────────────────────────────────────────────
def main():
    """CLI entry point."""
    parser = argparse.ArgumentParser(
        description='Pure Python ELF firmware image builder')
    parser.add_argument('elf_file', help='Input ELF file')
    parser.add_argument('--version', '-V', default='1.0.0',
                        help='Firmware version string (default: 1.0.0)')
    parser.add_argument('--its-file', help='Output .its file path '
                        '(default: <elf_basename>_os.its)')
    parser.add_argument('--description', default='Luban-lite firmware',
                        help='Firmware description in ITS (default: "Luban-lite firmware")')
    parser.add_argument('--hash', nargs='*', default=None, dest='hash_algos',
                        help='Hash algorithms (default: crc32 md5; '
                             'pass with no args to disable)')
    args, _unknown = parser.parse_known_args()

    its_file = args.its_file
    if its_file is None:
        its_file = args.elf_file[:-4] + '_os.its'

    hash_algos = args.hash_algos if args.hash_algos is not None else ["crc32", "md5"]

    elf2its(
        elf_file=args.elf_file,
        version=args.version,
        its_file=its_file,
        description=args.description,
        hash_algos=hash_algos,
    )
