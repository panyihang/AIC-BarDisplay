#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
# Thin CLI wrapper around the aic_private_resource package.
# Usage:  python3 mk_private_resource.py -c <config> -o <output>
#     or: python3 mk_private_resource.py -l <cfg1>,<cfg2> -o <output>
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from aic_private_resource import main  # noqa: E402

if __name__ == '__main__':
    main()
