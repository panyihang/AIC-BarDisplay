# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._dump — ITB file inspection / decompilation.

Provides ``dump_itb()`` which reads a binary ITB file and outputs
either a human-readable DTS text (like ``dtc -I dtb -O dts``) or
a FIT structure summary (like ``mkimage -l``).
"""
import os
import struct
import time
from typing import Optional

import fdt


# ── Helpers ─────────────────────────────────────────────────────────

def _get_prop_value(node, name: str, default=None):
    """Get the first value from a property, or *default*."""
    prop = node.get_property(name)
    if prop is None:
        return default
    try:
        return prop[0]
    except (IndexError, TypeError):
        return default


def _get_prop_bytes(node, name: str) -> Optional[bytes]:
    """Get raw bytes from a property, or *None*."""
    prop = node.get_property(name)
    if prop is None:
        return None
    if isinstance(prop, fdt.PropBytes):
        return bytes(prop.data)
    if isinstance(prop, fdt.PropWords):
        return b''.join(struct.pack(">I", v) for v in prop.data)
    return None


def _hex_or_str(value) -> str:
    """Format a value for summary display."""
    if isinstance(value, int):
        return f"0x{value:08X}"
    return str(value)


# ── External Data Handling ───────────────────────────────────────────

def _extract_external_images(
    fdt_obj: fdt.FDT,
    raw_data: bytes,
    extract_dir: Optional[str] = None,
) -> None:
    """Resolve external data references in image nodes.

    For each image with ``data-position`` / ``data-offset`` +
    ``data-size``, read the data from *raw_data* and either:
    - Replace with a ``data`` property (in-memory), or
    - Export to a ``.bin`` file and set ``/incbin/`` reference.
    """
    images_node = fdt_obj.get_node("images")
    if images_node is None:
        return

    # Infer data base offset from image properties.
    # Prefer data-position (absolute file offset) if available.
    # Fall back to data-offset (relative to external data start).
    data_base = None
    dtb_raw_size = fdt_obj.header.total_size
    min_offset = None
    for img_node in images_node.nodes:
        data_pos = _get_prop_value(img_node, "data-position")
        data_off = _get_prop_value(img_node, "data-offset")
        if data_pos is not None:
            data_base = 0  # positions are absolute
            break
        if data_off is not None:
            if min_offset is None or data_off < min_offset:
                min_offset = data_off
    if data_base is None and min_offset is not None:
        # Find smallest valid alignment after DTB
        for align in [4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096]:
            candidate = (dtb_raw_size + align - 1) & ~(align - 1)
            if candidate + min_offset <= len(raw_data):
                data_base = candidate
                break
        if data_base is None:
            data_base = dtb_raw_size

    for img_node in list(images_node.nodes):
        data_size = _get_prop_value(img_node, "data-size")
        data_pos = _get_prop_value(img_node, "data-position")
        data_off = _get_prop_value(img_node, "data-offset")

        if data_size is None:
            continue

        # Determine absolute offset in the file
        if data_pos is not None:
            abs_offset = data_pos
        elif data_off is not None and data_base is not None:
            abs_offset = data_base + data_off
        else:
            continue

        if abs_offset + data_size > len(raw_data):
            print(f"Warning: external data for '{img_node.name}' "
                  f"extends beyond file (offset={abs_offset:#x}, "
                  f"size={data_size:#x})")
            continue

        ext_data = raw_data[abs_offset:abs_offset + data_size]

        # Remove external-data properties
        img_node.remove_property("data-position")
        img_node.remove_property("data-offset")
        img_node.remove_property("data-size")

        if extract_dir:
            # Export to file and create /incbin/ reference
            os.makedirs(extract_dir, exist_ok=True)
            bin_name = img_node.name + '.bin'
            bin_path = os.path.join(extract_dir, bin_name)
            with open(bin_path, 'wb') as fh:
                fh.write(ext_data)
            img_node.append(
                fdt.PropIncBin("data", ext_data, bin_name,
                               extract_dir))
        else:
            # Inline the data back as a property
            img_node.set_property("data", ext_data)


# ── DTS Output ──────────────────────────────────────────────────────

def _output_dts(
    fdt_obj: fdt.FDT,
    dts_output: Optional[str] = None,
) -> None:
    """Write or print the DTS representation of the FDT."""
    dts_text = fdt_obj.to_dts()
    if dts_output:
        with open(dts_output, 'w', encoding='utf-8') as fh:
            fh.write(dts_text)
        print(f"DTS written to {dts_output}")
    else:
        print(dts_text)


# ── Summary Output ──────────────────────────────────────────────────

def _print_summary(fdt_obj: fdt.FDT) -> None:
    """Print a FIT structure summary (like ``mkimage -l``)."""
    desc = _get_prop_value(fdt_obj, "description", "")
    ts = _get_prop_value(fdt_obj, "timestamp", 0)
    ts_str = time.strftime(
        "%a %b %d %H:%M:%S %Y", time.localtime(ts)) if ts else "N/A"

    print(f"FIT description: {desc}")
    print(f"Created:         {ts_str}")

    # Default configuration
    conf_node = None
    try:
        conf_node = fdt_obj.get_node("configurations")
        def_conf = _get_prop_value(conf_node, "default", "")
        print(f"Default config:  {def_conf}")
    except ValueError:
        pass

    # Images
    try:
        images_node = fdt_obj.get_node("images")
    except ValueError:
        images_node = None

    if images_node:
        for n, img in enumerate(images_node.nodes):
            img_type = _get_prop_value(img, "type", "unknown")
            arch = _get_prop_value(img, "arch", "")
            os_name = _get_prop_value(img, "os", "")
            comp = _get_prop_value(img, "compression", "none")
            load = _get_prop_value(img, "load")
            entry = _get_prop_value(img, "entry")

            # Data size: from data property or data-size
            data_prop = img.get_property("data")
            if data_prop is not None:
                if isinstance(data_prop, fdt.PropBytes):
                    data_len = len(data_prop)
                elif isinstance(data_prop, fdt.PropWords):
                    data_len = len(data_prop) * 4
                else:
                    data_len = 0
            else:
                data_len = _get_prop_value(img, "data-size", 0)

            print(f"\n Image {n} ({img.name})")
            print(f"  Type:        {img_type}")
            if arch:
                print(f"  Arch:        {arch}")
            if os_name:
                print(f"  OS:          {os_name}")
            print(f"  Compression: {comp}")
            if load is not None:
                print(f"  Load:        {_hex_or_str(load)}")
            if entry is not None:
                print(f"  Entry:       {_hex_or_str(entry)}")
            print(f"  Data Size:   {data_len} bytes "
                  f"({data_len / 1024:.1f} kB)")

            # Hash / signature / cipher sub-nodes
            for sub in img.nodes:
                sub_name = sub.name
                algo = _get_prop_value(sub, "algo", "")
                if sub_name.startswith("hash"):
                    val_prop = sub.get_property("value")
                    val_info = ""
                    if val_prop:
                        if isinstance(val_prop, fdt.PropBytes):
                            val_info = f" ({len(val_prop)} bytes)"
                        elif isinstance(val_prop, fdt.PropWords):
                            val_info = f" ({len(val_prop) * 4} bytes)"
                        else:
                            val_info = f" (present)"
                    print(f"  Hash:        {algo}{val_info}")
                elif sub_name.startswith("signature"):
                    print(f"  Signature:   {algo}")
                elif sub_name.startswith("cipher"):
                    print(f"  Cipher:      {algo}")

    # Configurations
    if conf_node:
        for n, cfg in enumerate(conf_node.nodes):
            desc_val = _get_prop_value(cfg, "description", "")
            print(f"\n Configuration {n} ({cfg.name})")
            print(f"  Description: {desc_val}")
            # List referenced images
            for prop in cfg.props:
                if prop.name in ("description", "signature-1",
                                 "signature"):
                    continue
                if isinstance(prop, fdt.PropStrings):
                    refs = ', '.join(str(v) for v in prop.data)
                    print(f"  {prop.name}: {refs}")

            # Signature sub-nodes
            for sub in cfg.nodes:
                if sub.name.startswith("signature"):
                    algo = _get_prop_value(sub, "algo", "")
                    print(f"  Signature:   {algo}")


# ── Public API ──────────────────────────────────────────────────────

def dump_itb(
    itb_file: str,
    *,
    dts_output: Optional[str] = None,
    extract_dir: Optional[str] = None,
    summary: bool = False,
) -> None:
    """Read and inspect an ITB (FIT) image file.

    Args:
        itb_file:    Path to the input ``.itb`` file.
        dts_output:  If set, write DTS text to this file path
                     instead of stdout.
        extract_dir: If set, extract external image data to this
                     directory as ``.bin`` files.
        summary:     If *True*, print a summary instead of full DTS.
    """
    with open(itb_file, 'rb') as fh:
        raw_data = fh.read()

    fdt_obj = fdt.parse_dtb(raw_data)

    # Resolve external data references
    _extract_external_images(fdt_obj, raw_data, extract_dir)

    if summary:
        _print_summary(fdt_obj)
    else:
        _output_dts(fdt_obj, dts_output)
