#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
# Wu Dehuang <dehuang.wu@artinchip.com>
#
#
# Tools to generate encrypted Pre-Boot-Program

import os
import re
import sys
import math
import json
import argparse
from Cryptodome.Cipher import DES
from Cryptodome.Cipher import AES


def round_up(x, y):
    return int((x + y - 1) / y) * y


def do_pbp_encrypt(pbp, pnkfile):
    """
    """
    len_32bit = len(pbp)

    seed = len_32bit.to_bytes(4, byteorder='little', signed=False)
    seed_128bit = seed + seed + seed + seed
    # Step1: PNK is DES key, use it to generate AES key
    try:
        with open(pnkfile, "rb") as f:
            pnkval = f.read()
            if len(pnkval) != 8:
                print("PNK is a DES key, should be 64 bits length.")
                sys.exit(1)
    except IOError:
        print('Failed to open {}.'.format(pnkfile))
        sys.exit(1)
    des_cipher = DES.new(pnkval, DES.MODE_ECB)
    aeskey = des_cipher.encrypt(bytes(seed_128bit))
    aes_cipher = AES.new(aeskey, AES.MODE_ECB)
    enc_bytes = aes_cipher.encrypt(pbp)
    return enc_bytes


def calc_checksum(pbpbytes):
    length = len(pbpbytes)
    offset = 0
    total = 0
    while offset < length:
        val = int.from_bytes(pbpbytes[offset: offset + 4], byteorder='little', signed=False)
        total = total + val
        offset = offset + 4
    return (~total) & 0xFFFFFFFF


def gen_pbp_with_header(pbpbytes, pnk, magic):
    """
        PBP format:
        struct pbp_header {
            char magic[4]; // "PBP2"
            u32  checksum; // Secure boot it can be set to 0
            u32  head_ver;
            u32  sign_offset; // Offset from PBP header start to the pbp signature
            char pad[16];
            .... PBP data; // should align to 256 byte
            u8 sign[256];
    """
    headerlen = 32
    srclen = len(pbpbytes)
    # 256 bytes alignment for AES encryption and signature
    padlen = round_up(srclen + headerlen, 256)
    if padlen != (srclen + headerlen):
        pbpbytes = pbpbytes + bytearray(padlen - (srclen + headerlen))

    if pnk is not None:
        pbpbytes = do_pbp_encrypt(pbpbytes, pnk)

    # Set to 0 first
    csval = 0
    head_ver = 0x00010001
    sign_offset = padlen
    head_bytes = magic.encode(encoding="utf-8")
    head_bytes = head_bytes + csval.to_bytes(4, byteorder='little', signed=False)
    head_bytes = head_bytes + head_ver.to_bytes(4, byteorder='little', signed=False)
    head_bytes = head_bytes + sign_offset.to_bytes(4, byteorder='little', signed=False)
    head_bytes = head_bytes + bytearray(16)

    pbpbytes = head_bytes + pbpbytes
    csval = calc_checksum(pbpbytes)
    cs_bytes = csval.to_bytes(4, byteorder='little', signed=False)
    pbpbytes = pbpbytes[0:4] + cs_bytes + pbpbytes[8:]
    # Verify the checksum value
    csval = calc_checksum(pbpbytes)
    if csval != 0:
        print("Checksum is error: {}".format(csval))
        sys.exit(1)
    return pbpbytes


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--magic", type=str,
                        help="Magic value")
    parser.add_argument("-k", "--pnk", type=str,
                        help="eFuse PNK Key")
    parser.add_argument("-i", "--infile", type=str,
                        help="Input PBP file")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output encrypted PBP file")
    args = parser.parse_args()
    magic = "PBP2"
    # If user not specified data directory, use current directory as default
    if args.infile is None:
        print('--infile is required.')
        sys.exit(1)
    if args.outfile is None:
        print('--outfile is required.')
        sys.exit(1)
    if args.magic is not None:
        magic = args.magic
    try:
        with open(args.infile, "rb") as f:
            pbpbytes = f.read()
            pbpbytes = gen_pbp_with_header(pbpbytes, args.pnk, magic)
    except IOError:
        print('Failed to open {}.'.format(args.infile))
        sys.exit(1)

    try:
        with open(args.outfile, "wb") as f:
            f.write(pbpbytes)
    except IOError:
        print('Failed to open {}.'.format(args.outfile))
        sys.exit(1)
