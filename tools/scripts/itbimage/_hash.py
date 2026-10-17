# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._hash — Hash computation for FIT image nodes.

Corresponds to C ``fit_image_process_hash()`` in
``u-boot/tools/image-host.c``.
"""
import hashlib
import struct
import zlib
from typing import Optional

import fdt


# ── Supported Algorithms ─────────────────────────────────────────────

_HASH_ALGOS = {
    'crc32':   ('crc32',  4),
    'md5':     ('md5',    16),
    'sha1':    ('sha1',   20),
    'sha256':  ('sha256', 32),
    'sha384':  ('sha384', 48),
    'sha512':  ('sha512', 64),
}


def _compute_hash(algo: str, data: bytes) -> bytes:
    """Compute hash of *data* using the specified algorithm.

    Returns the raw hash bytes.
    """
    algo_lower = algo.lower()
    if algo_lower not in _HASH_ALGOS:
        raise ValueError(f"Unsupported hash algorithm: {algo}")

    if algo_lower == 'crc32':
        crc = zlib.crc32(data) & 0xFFFFFFFF
        return struct.pack(">I", crc)

    h = hashlib.new(algo_lower)
    h.update(data)
    return h.digest()


# ── Helpers ─────────────────────────────────────────────────────────

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
    prop = node.get_property(name)
    if prop is None:
        return default
    try:
        return prop[0]
    except (IndexError, TypeError):
        return default


# ── Public API ──────────────────────────────────────────────────────

def compute_image_hashes(fdt_obj: fdt.FDT) -> None:
    """Compute and set hash values for all ``hash*`` sub-nodes
    under ``/images``.

    For each image node, iterates over its child nodes.  If a
    child's name starts with ``hash``, reads the ``algo`` property,
    computes the hash of the image ``data``, and writes the result
    to the hash node's ``value`` property.
    """
    images_node = fdt_obj.get_node("images")
    if images_node is None:
        return

    for img_node in images_node.nodes:
        data = _get_image_data(img_node)
        if not data:
            continue

        for sub_node in list(img_node.nodes):
            if not sub_node.name.startswith("hash"):
                continue

            algo = _get_prop_str(sub_node, "algo")
            if not algo:
                print(f"Warning: hash node '{sub_node.name}' in "
                      f"'{img_node.name}' has no algo property")
                continue

            try:
                hash_value = _compute_hash(algo, data)
            except ValueError as exc:
                print(f"Warning: {exc}")
                continue

            sub_node.set_property("value", hash_value)
