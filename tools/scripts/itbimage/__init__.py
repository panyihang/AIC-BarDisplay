# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage — Pure Python FIT (Flattened Image Tree) image builder & inspector.

Replaces the C-based ``mkimage`` tool from U-Boot.
Supports ITS→ITB conversion, external data extraction, hashing,
RSA signing, AES-CBC / ChaCha20 encryption, and ITB inspection.

Public API
----------
.. autofunction:: build_itb
.. autofunction:: dump_itb

CLI usage (via wrapper script or ``python -m itbimage``)::

    itbimage build  [-f ITS] [-E] [-B ALIGN] [-k KEYDIR] [-K KEYDEST] [-r] OUTPUT
    itbimage dump   [-o DTS_OUT] [-x EXTRACT_DIR] [--summary] ITB_FILE
"""
from itbimage._core import build_itb, main
from itbimage._dump import dump_itb

__all__ = ['build_itb', 'dump_itb', 'main']
__version__ = '1.0.0'
