# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
elf2image — Pure Python ELF firmware image builder.

Converts ELF binaries into .its (Image Tree Source) packages with
extracted segment binaries.  No external ``readelf`` or ``objcopy``
required — uses only the Python standard library.

Public API
----------
.. autofunction:: elf2its

CLI usage (via wrapper script or ``python -m elf2image``)::

    python elf_parse_pure.py <elf_file> [--version V] [--its-file PATH]
                                        [--description TEXT] [--hash algo ...]
"""
from elf2image._core import elf2its, main

__all__ = ['elf2its', 'main']
__version__ = '1.0.0'
