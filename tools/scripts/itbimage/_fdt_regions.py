# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._fdt_regions — DTB region extraction.

Python equivalent of the C function ``fdt_find_regions()`` in
``u-boot/boot/fdt_region.c``.

Used by configuration signing to determine which parts of the DTB
must be hashed and signed.
"""
import struct
from typing import List, Tuple

from fdt.header import (
    Header, DTB_BEGIN_NODE, DTB_END_NODE, DTB_PROP, DTB_NOP, DTB_END)

# Type alias for a region: (absolute_offset, size)
Region = Tuple[int, int]

_MAX_DEPTH = 32


def _extract_string(data: bytes, offset: int) -> str:
    """Extract a NUL-terminated string from *data* at *offset*."""
    end = data.index(b'\x00', offset)
    return data[offset:end].decode('ascii', errors='replace')


def find_regions(
    dtb_data: bytes,
    inc_paths: List[str],
    exc_props: List[str],
    *,
    add_string_tab: bool = True,
) -> Tuple[List[Region], List[str]]:
    """Find byte regions in a DTB corresponding to specified node paths.

    This is a Python port of the C ``fdt_find_regions()`` function.

    Args:
        dtb_data:        Raw DTB binary.
        inc_paths:       Node paths to include (e.g.
                         ``['/images/seg0', '/configurations/conf-1']``).
        exc_props:       Property names to exclude (e.g.
                         ``['data', 'data-size', 'data-position',
                         'data-offset']``).
        add_string_tab:  If *True*, the last region includes the
                         strings block.

    Returns:
        A tuple ``(regions, hashed_nodes)`` where:
        - *regions* is a list of ``(offset, size)`` tuples.
        - *hashed_nodes* is the list of node paths that were
          actually included (for writing the ``hashed-nodes``
          property).
    """
    header = Header.parse(dtb_data)
    base = header.off_dt_struct
    strings_base = header.off_dt_strings

    # Normalise include paths (strip trailing '/')
    inc_set = set(p.rstrip('/') for p in inc_paths)
    exc_set = set(exc_props)

    regions: List[Region] = []
    hashed_nodes: List[str] = []

    # Stack tracking
    stack_want: List[int] = [0] * _MAX_DEPTH
    depth = -1
    want = 0
    start = -1
    path_parts: List[str] = []

    idx = base
    end_idx = base + (header.size_dt_struct
                      if header.size_dt_struct
                      else len(dtb_data) - base)

    while idx < end_idx:
        tag = struct.unpack_from(">I", dtb_data, idx)[0]
        offset = idx

        if tag == DTB_BEGIN_NODE:
            idx += 4
            name = _extract_string(dtb_data, idx)
            idx = (idx + len(name) + 4) & ~3  # align to 4
            if not name:
                name = ''
            depth += 1
            if depth >= _MAX_DEPTH:
                raise ValueError("FDT depth exceeded")
            path_parts.append(name)
            current_path = '/' + '/'.join(
                p for p in path_parts if p)

            stack_want[depth] = want
            # Check if current path matches an include path
            if current_path.rstrip('/') in inc_set:
                want = 2
                hashed_nodes.append(current_path)
            elif want:
                want -= 1
            include = want

            # Region tracking
            if include and start == -1:
                # Try merge with previous region
                if (regions and offset ==
                        regions[-1][0] + regions[-1][1]):
                    start = regions[-1][0]
                    regions.pop()
                else:
                    start = offset
            if not include and start != -1:
                regions.append((start, offset - start))
                start = -1

        elif tag == DTB_END_NODE:
            idx += 4
            include = want
            if depth >= 0:
                want = stack_want[depth]
                depth -= 1
            if path_parts:
                path_parts.pop()

            if include and start == -1:
                if (regions and offset ==
                        regions[-1][0] + regions[-1][1]):
                    start = regions[-1][0]
                    regions.pop()
                else:
                    start = offset
            if not include and start != -1:
                regions.append((start, idx - start))
                start = -1

        elif tag == DTB_PROP:
            idx += 4
            prop_size, prop_str_pos = struct.unpack_from(
                ">II", dtb_data, idx)
            idx += 8
            if header.version < 16 and prop_size >= 8:
                idx = (idx + 7) & ~7
            prop_name = _extract_string(
                dtb_data, strings_base + prop_str_pos)
            idx += prop_size
            idx = (idx + 3) & ~3  # align

            include = 1 if want >= 2 else 0
            stop_at = offset
            if prop_name in exc_set:
                include = 0

            if include and start == -1:
                if (regions and offset ==
                        regions[-1][0] + regions[-1][1]):
                    start = regions[-1][0]
                    regions.pop()
                else:
                    start = offset
            if not include and start != -1:
                regions.append((start, stop_at - start))
                start = -1

        elif tag == DTB_NOP:
            idx += 4
            include = 1 if want >= 2 else 0
            stop_at = offset

            if include and start == -1:
                start = offset
            if not include and start != -1:
                regions.append((start, stop_at - start))
                start = -1

        elif tag == DTB_END:
            idx += 4
            include = 1  # always include END tag

            if include and start == -1:
                start = offset
            # END tag: close off final region
            # (fall through to the end-of-loop handling below)
            break

        else:
            raise ValueError(f"Unknown FDT tag: {tag:#x}")

    # Final region: from start to end of struct block + string table
    if start != -1:
        final_size = idx - start
        if add_string_tab:
            final_size += header.size_dt_strings or 0
        regions.append((start, final_size))

    return regions, hashed_nodes


def compute_regions_hash(
    dtb_data: bytes,
    regions: List[Region],
    algo: str = 'sha256',
) -> bytes:
    """Hash the concatenated region data.

    Args:
        dtb_data: Full DTB binary.
        regions:  List of ``(offset, size)`` from :func:`find_regions`.
        algo:     Hash algorithm name (e.g. ``'sha256'``).

    Returns:
        Raw hash digest bytes.
    """
    import hashlib

    h = hashlib.new(algo)
    for off, sz in regions:
        h.update(dtb_data[off:off + sz])
    return h.digest()
