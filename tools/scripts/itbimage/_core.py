# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._core — FIT image building core logic and CLI entry point.

Public API (re-exported from ``__init__.py``):
    build_itb()  — full pipeline: parse ITS → hash → sign → encrypt → ITB
    main()       — CLI entry point with build / dump sub-commands
"""
import argparse
import os
import struct
import sys
import time
from typing import Dict, List, Optional, Tuple

# Ensure sibling packages (fdt, Cryptodome, gmssl) are importable
_SCRIPTS_DIR = os.path.normpath(
    os.path.join(os.path.dirname(__file__), '..'))
if _SCRIPTS_DIR not in sys.path:
    sys.path.insert(0, _SCRIPTS_DIR)

import fdt  # noqa: E402


# ── Helpers ─────────────────────────────────────────────────────────

def _align_up(value: int, align: int) -> int:
    """Round *value* up to the nearest multiple of *align*."""
    if align <= 0:
        return value
    return (value + align - 1) & ~(align - 1)


def _get_image_data(node) -> bytes:
    """Extract raw bytes from an image node's ``data`` property."""
    prop = node.get_property("data")
    if prop is None:
        return b''
    if isinstance(prop, fdt.PropBytes):
        return bytes(prop.data)
    if isinstance(prop, fdt.PropWords):
        return b''.join(struct.pack(">I", v) for v in prop.data)
    raise ValueError(
        f"Unsupported data property type in node '{node.name}'")


def _get_prop_str(node, name: str, default: str = '') -> str:
    """Get the first string value of a property, or *default*."""
    prop = node.get_property(name)
    if prop is None:
        return default
    try:
        return prop[0]
    except (IndexError, TypeError):
        return default


def _get_prop_int(node, name: str, default: int = 0) -> int:
    """Get the first integer value of a property, or *default*."""
    prop = node.get_property(name)
    if prop is None:
        return default
    if isinstance(prop, fdt.PropWords) and len(prop) > 0:
        return prop[0]
    return default


# ── ITS Parsing ──────────────────────────────────────────────────────

def _parse_its(its_file: str) -> fdt.FDT:
    """Parse an ITS file into an FDT object.

    Uses :func:`fdt.parse_dts` which already handles ``/incbin/``
    directives to embed binary data from referenced files.
    """
    its_dir = os.path.dirname(os.path.abspath(its_file))
    with open(its_file, 'r', encoding='utf-8') as fh:
        text = fh.read()
    return fdt.parse_dts(text, its_dir)


# ── Timestamp ────────────────────────────────────────────────────────

def _set_timestamp(fdt_obj: fdt.FDT) -> None:
    """Add or update the ``timestamp`` property on the root node."""
    ts = int(time.time())
    fdt_obj.set_property("timestamp", ts)


# ── External Data Extraction ─────────────────────────────────────────

def _extract_external_data(
    fdt_obj: fdt.FDT,
    dtb_data: bytes,
    block_align: int,
) -> Tuple[bytes, bytes]:
    """Move image data out of the DTB and into an external buffer.

    Corresponds to the C function ``fit_extract_data()`` in
    ``u-boot/tools/fit_image.c``.

    1. Collect image data from ``/images`` nodes and remove their
       ``data`` properties.
    2. Serialize the modified FDT → DTB.
    3. Align DTB size to *block_align*.
    4. Build external data buffer with per-image alignment.
    5. Set ``data-offset`` + ``data-size`` on each image node.
    6. Re-serialize the FDT → final DTB with correct offsets.

    Returns ``(dtb_bytes, ext_data_bytes)``.
    """
    if block_align <= 0:
        block_align = 4

    images_node = fdt_obj.get_node("images")
    if images_node is None or not images_node.nodes:
        return dtb_data, b''

    # ── Phase 1: collect data + strip 'data' properties ─────────
    image_entries: List[Tuple[object, bytes, int]] = []
    for img_node in list(images_node.nodes):
        data = _get_image_data(img_node)
        if not data:
            continue
        img_node.remove_property("data")
        # Per-image alignment: use block_align if set
        img_align = block_align
        image_entries.append((img_node, data, img_align))

    if not image_entries:
        # No images to extract; re-serialize and return
        new_dtb = fdt_obj.to_dtb(version=17)
        return new_dtb, b''

    # ── Phase 2: first serialisation to estimate DTB size ────────
    # Add temporary placeholder offset/size/position so we know the
    # final property sizes.  Values will be overwritten in Phase 4.
    for img_node, data, _ in image_entries:
        img_node.set_property("data-offset", 0)
        img_node.set_property("data-size", len(data))
        img_node.set_property("data-position", 0)
    temp_dtb = fdt_obj.to_dtb(version=17)
    temp_size = len(temp_dtb)

    # ── Phase 3: compute aligned boundary & per-image offsets ─────
    aligned_boundary = _align_up(temp_size, block_align)
    buf = bytearray()
    offsets: List[int] = []
    prev_len = 0
    for _img_node, data, img_align in image_entries:
        # Align buf_ptr BEFORE placing this image (mirrors C code)
        aligned_ptr = _align_up(len(buf), img_align)
        pad = aligned_ptr - len(buf)
        if pad:
            buf.extend(b'\x00' * pad)
        offsets.append(len(buf))
        buf.extend(data)
        prev_len = len(data)
    # Final alignment for trailing image
    final_ptr = _align_up(len(buf), block_align)
    final_pad = final_ptr - len(buf)
    if final_pad:
        buf.extend(b'\x00' * final_pad)

    # ── Phase 4: write real offset / size / position properties ─────
    for (img_node, data, _), offset in zip(image_entries, offsets):
        img_node.set_property("data-offset", offset)
        img_node.set_property("data-size", len(data))
        # Also write absolute data-position for easy parsing
        img_node.set_property("data-position", aligned_boundary + offset)

    # ── Phase 5: final serialisation ──────────────────────────────
    final_dtb = fdt_obj.to_dtb(version=17)
    # Pad DTB to aligned boundary
    if len(final_dtb) < aligned_boundary:
        final_dtb += b'\x00' * (aligned_boundary - len(final_dtb))

    return final_dtb, bytes(buf)


# ── Main Build Function ──────────────────────────────────────────────

def build_itb(
    its_file: str,
    output_file: str,
    *,
    external_data: bool = False,
    block_align: int = 0,
    keydir: Optional[str] = None,
    keydest: Optional[str] = None,
    require_keys: bool = False,
) -> None:
    """Build an ITB (FIT) image from an ITS source file.

    This is the main public API, equivalent to the C ``mkimage`` tool.

    Args:
        its_file:      Path to the input ``.its`` file.
        output_file:   Path for the output ``.itb`` file.
        external_data: If *True*, extract image data outside the DTB
                       (``-E`` flag).
        block_align:   Block alignment for external data (``-B`` value).
        keydir:        Directory containing signing / encryption keys
                       (``-k`` flag).
        keydest:       Target DTB for public-key injection (``-K`` flag).
        require_keys:  Mark injected keys as required (``-r`` flag).
    """
    # 1. Parse ITS → FDT object
    fdt_obj = _parse_its(its_file)

    # 2. Set timestamp
    _set_timestamp(fdt_obj)

    # 3. Encryption (AES-CBC / ChaCha20) — before hashing
    if keydir:
        try:
            from itbimage._cipher import cipher_images
            cipher_images(fdt_obj, keydir)
        except ImportError:
            pass

    # 4. Compute hashes (image nodes)
    try:
        from itbimage._hash import compute_image_hashes
        compute_image_hashes(fdt_obj)
    except ImportError:
        pass

    # 5. RSA signing (image + configuration nodes)
    if keydir:
        try:
            from itbimage._sign import (
                sign_images, sign_configurations)
            sign_images(fdt_obj, keydir)
            sign_configurations(fdt_obj, keydir)
        except ImportError:
            pass

    # 6. Serialize to DTB
    dtb_data = fdt_obj.to_dtb(version=17)

    # 7. External data extraction (-E -B)
    if external_data:
        dtb_data, ext_data = _extract_external_data(
            fdt_obj, dtb_data, block_align)
    else:
        ext_data = b''

    # 8. Write output file
    with open(output_file, 'wb') as fh:
        fh.write(dtb_data)
        fh.write(ext_data)

    # 9. Public-key injection into DTB (-K)
    if keydest and keydir:
        try:
            from itbimage._sign import inject_pubkeys
            inject_pubkeys(keydir, keydest, require_keys)
        except ImportError:
            pass


# ── CLI Entry Point ──────────────────────────────────────────────────

def _build_cmd(args: argparse.Namespace) -> None:
    """Handle the ``build`` sub-command."""
    block_align = 0
    if args.block_align:
        block_align = int(args.block_align, 0)
    build_itb(
        its_file=args.its_file,
        output_file=args.output,
        external_data=args.external,
        block_align=block_align,
        keydir=args.keydir,
        keydest=args.keydest,
        require_keys=args.require_keys,
    )


def _dump_cmd(args: argparse.Namespace) -> None:
    """Handle the ``dump`` sub-command."""
    from itbimage._dump import dump_itb
    dump_itb(
        itb_file=args.itb_file,
        dts_output=args.output,
        extract_dir=args.extract,
        summary=args.summary,
    )


def main():
    """CLI entry point with ``build`` and ``dump`` sub-commands."""
    parser = argparse.ArgumentParser(
        prog='itbimage',
        description='Pure Python FIT image builder & inspector')
    sub = parser.add_subparsers(dest='command')

    # ── build ────────────────────────────────────────────────────
    p_build = sub.add_parser(
        'build', help='Build ITB from ITS source')
    p_build.add_argument(
        '-f', dest='its_file', required=True,
        help='Input ITS file')
    p_build.add_argument(
        '-E', dest='external', action='store_true',
        help='Extract image data outside the DTB')
    p_build.add_argument(
        '-B', dest='block_align', default=None,
        help='Block alignment for external data (e.g. 0x800)')
    p_build.add_argument(
        '-k', dest='keydir', default=None,
        help='Directory with signing / encryption keys')
    p_build.add_argument(
        '-K', dest='keydest', default=None,
        help='Target DTB for public-key injection')
    p_build.add_argument(
        '-r', dest='require_keys', action='store_true',
        help='Mark injected keys as required')
    p_build.add_argument(
        'output', help='Output ITB file')

    # ── dump ─────────────────────────────────────────────────────
    p_dump = sub.add_parser(
        'dump', help='Inspect / decompile an ITB file')
    p_dump.add_argument(
        'itb_file', help='Input ITB file')
    p_dump.add_argument(
        '-o', dest='output', default=None,
        help='Write DTS output to file (default: stdout)')
    p_dump.add_argument(
        '-x', dest='extract', default=None,
        help='Extract external data to this directory')
    p_dump.add_argument(
        '--summary', action='store_true',
        help='Print FIT structure summary instead of DTS')

    args, _unknown = parser.parse_known_args()

    if args.command == 'build':
        _build_cmd(args)
    elif args.command == 'dump':
        _dump_cmd(args)
    else:
        parser.print_help()
        sys.exit(1)
