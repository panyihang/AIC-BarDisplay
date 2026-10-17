# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
aic_private_resource — Pure Python AIC private resource builder.

Generates binary private resource data (DRAM/PSRAM/UART/JTAG/partition
configs) from JSON configuration files.  No external tools required —
uses only the Python standard library.

Public API
----------
.. autofunction:: parse_config
.. autofunction:: build_private_data
.. autofunction:: build_from_files
"""
from aic_private_resource._core import (
    build_from_files,
    build_private_data,
    main,
    parse_config,
)

__all__ = ['build_from_files', 'build_private_data', 'main', 'parse_config']
__version__ = '1.0.0'
