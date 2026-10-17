#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
# Dehuang Wu <dehuang.wu@artinchip.com>

import os
import sys
import re
import math
import zlib
import json
import struct
import argparse
import platform
import subprocess
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elf2image import elf2its  # noqa: E402
from aic_private_resource import (  # noqa: E402
    parse_config as _priv_parse_config,
    build_private_data as _priv_build,
)
from collections import namedtuple  # noqa: E402
from collections import OrderedDict  # noqa: E402
from Cryptodome.PublicKey import RSA  # noqa: E402
from Cryptodome.Hash import MD5  # noqa: E402
from Cryptodome.Hash import SHA256  # noqa: E402
from Cryptodome.Hash import HMAC  # noqa: E402
from Cryptodome.Cipher import AES  # noqa: E402
from Cryptodome.Cipher import ChaCha20  # noqa: E402
from Cryptodome.Util import Counter  # noqa: E402
from Cryptodome.Signature import PKCS1_v1_5  # noqa: E402
import binascii  # noqa: E402
import asn1crypto.core  # noqa: E402
import gmssl.sm2 as SM2  # noqa: E402
import gmssl.sm3 as SM3  # noqa: E402
import gmssl.sm4 as SM4  # noqa: E402
import gmssl.func as func  # noqa: E402

DATA_ALIGNED_SIZE = 2048
META_ALIGNED_SIZE = 512

# Signature algorithm to (algo_id, sign_length) mapping
_SIGN_ALGO_MAP = {
    "rsa,2048": {"algo": 1, "len": 256},
    "sm2": {"algo": 2, "len": 64},
    "hmac,sha256": {"algo": 3, "len": 32},
}

# Encryption algorithm to algo_id mapping
_ENC_ALGO_MAP = {
    "aes-128-cbc": 1,
    "sm4-ecb": 2,
    "sm4-cbc": 3,
    "aes-128-ctr": 4,
    "chacha20": 5,
}

# Encryption algorithm to (key_size, iv_size) mapping
_ENC_KEY_IV_MAP = {
    "aes-128-cbc": (16, 16),
    "aes-128-ctr": (16, 16),
    "aes-192-cbc": (24, 16),
    "aes-256-cbc": (32, 16),
    "sm4-ecb": (16, 0),
    "sm4-cbc": (16, 16),
    "chacha20": (32, 12),
}

# Checksum algorithm to auxiliary data length mapping
_CKSUM_ALGO_MAP = {
    "md5": 16,
    "sm3": 32,
}

# Whether or not to generate the image used by the burner
BURNER = False
VERBOSE = False

COLOR_BEGIN = "\033["
COLOR_RED = COLOR_BEGIN + "41;37m"
COLOR_YELLOW = COLOR_BEGIN + "43;30m"
COLOR_WHITE = COLOR_BEGIN + "47;30m"
COLOR_END = "\033[0m"


def pr_err(string):
    print(COLOR_RED + '*** ' + string + COLOR_END)


def pr_info(string):
    print(COLOR_WHITE + '>>> ' + string + COLOR_END)


def pr_warn(string):
    print(COLOR_YELLOW + '!!! ' + string + COLOR_END)


def parse_image_cfg(cfgfile):
    """ Load image configuration file
    Args:
        cfgfile: Configuration file name
    """

    with open(cfgfile, "r") as f:
        lines = f.readlines()
        jsonstr = ""
        for line in lines:
            sline = line.strip()
            if sline.startswith("//"):
                continue
            slash_start = sline.find("//")
            if slash_start > 0:
                jsonstr += sline[0:slash_start].strip()
            else:
                jsonstr += sline
        # Use OrderedDict is important, we need to iterate FWC in order.
        jsonstr = jsonstr.replace(",}", "}").replace(",]", "]")
        cfg = json.loads(jsonstr, object_pairs_hook=OrderedDict)
    return cfg


def dump_hex(msg, data, dlen):
    print("{}:".format(msg))
    for i in range(len(data)):
        if i > dlen:
            break
        if i > 0 and i % 16 == 0:
            print("")
        print("{} ".format(hex(data[i])), end="")
    print("")


def get_file_path(fpath, dirpath):
    if dirpath is not None and os.path.exists(dirpath + fpath):
        return dirpath + fpath
    if os.path.exists(fpath):
        return fpath
    return None


def _get_sign_len(cfg):
    """ Get signature length from config, return 0 if no signature """
    if "signature" not in cfg:
        return 0
    algo = cfg["signature"]["algo"]
    return _SIGN_ALGO_MAP.get(algo, {}).get("len", 0)


def _resolve_file_stat(filename, dirs, label):
    """ Search for a file in given directories and return its stat info

    Args:
        filename: File name to search
        dirs: List of directory paths to search in
        label: Label for error message if file not found
    """
    for d in dirs:
        if d is not None and os.path.exists(d + filename):
            statinfo = os.stat(d + filename)
            return statinfo
        if os.path.exists(filename):
            statinfo = os.stat(filename)
            return statinfo
    print("Error, {} is not found.".format(label))
    sys.exit(1)


def _add_file_size_entry(files, key, statinfo, align, sign_len_key=None, sign_len=0):
    """ Add file size and aligned size entry to the files dict

    Args:
        files: Dict to store file sizes
        key: Key prefix for the entries (e.g. "resource/private")
        statinfo: os.stat result for the file
        align: Alignment size for round_up
        sign_len_key: If set, also store sign_len under this key
        sign_len: Signature length value to store
    """
    files[key] = statinfo.st_size
    files["round({})".format(key)] = round_up(statinfo.st_size, align)
    if sign_len_key is not None:
        files["round({}/sign_len)".format(key)] = sign_len


def _add_resource_file_sizes(files, cfg_resource, key, dirs, default_align, sign_len=0):
    """ Add resource file size entries for a specific resource key

    Args:
        files: Dict to store file sizes
        cfg_resource: Resource section of config dict
        key: Resource key name (e.g. "private", "private2", "pubkey", "pbp", "pbp2")
        dirs: List of directory paths to search in
        default_align: Default alignment size when sign_len is 0
        sign_len: Signature length for alignment override
    """
    if key not in cfg_resource:
        return
    align = sign_len if sign_len else default_align
    # private2/pbp2 use simple directory search without get_file_path
    if key in ("private2", "pbp2"):
        for d in dirs:
            if d is not None and os.path.exists(d + cfg_resource[key]):
                statinfo = os.stat(d + cfg_resource[key])
                _add_file_size_entry(files, "resource/" + key, statinfo, align,
                                     "resource/" + key, sign_len)
                return
        print("Error, {} is not found.".format(cfg_resource[key]))
        sys.exit(1)
    else:
        statinfo = _resolve_file_stat(cfg_resource[key], dirs, cfg_resource[key])
        _add_file_size_entry(files, "resource/" + key, statinfo, align)


def aic_boot_get_resource_file_size(cfg, keydir, datadir):
    """ Get size of all resource files
    """

    files = {}
    dirs = [keydir, datadir]
    if "resource" in cfg:
        sign_len = _get_sign_len(cfg)
        res = cfg["resource"]
        _add_resource_file_sizes(files, res, "private", dirs, 32)
        _add_resource_file_sizes(files, res, "private2", dirs, 4, sign_len)
        _add_resource_file_sizes(files, res, "pubkey", dirs, 32)
        _add_resource_file_sizes(files, res, "pbp", dirs, 32)
        _add_resource_file_sizes(files, res, "pbp2", dirs, 16, sign_len)
    if "encryption" in cfg:
        if "iv" in cfg["encryption"]:
            statinfo = _resolve_file_stat(cfg["encryption"]["iv"], dirs,
                                          cfg["encryption"]["iv"])
            _add_file_size_entry(files, "encryption/iv", statinfo, 32)
    if "loader" in cfg:
        if "file" in cfg["loader"]:
            filepath = get_file_path(cfg["loader"]["file"], datadir)
            if filepath is not None:
                statinfo = os.stat(filepath)
                if statinfo.st_size > (4 * 1024 * 1024):
                    print("Loader size is too large")
                    sys.exit(1)
                files["loader/file"] = statinfo.st_size
                files["round(loader/file)"] = round_up(statinfo.st_size, 256)
            else:
                print("File {} is not exist".format(cfg["loader"]["file"]))
                sys.exit(1)
    return files


def aic_boot_calc_image_length(filesizes, sign_len):
    """ Calculate the boot image's total length
    """

    total_siz = filesizes["resource_start"]
    if "resource/pubkey" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pubkey)"]
    if "encryption/iv" in filesizes:
        total_siz = total_siz + filesizes["round(encryption/iv)"]
    if "resource/private2" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private2)"]
    elif "resource/private" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private)"]
    if "resource/pbp2" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pbp2)"]
    elif "resource/pbp" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pbp)"]
    total_siz = round_up(total_siz, 256)
    total_siz = total_siz + sign_len
    return total_siz


def aic_boot_calc_image_length_for_ext(filesizes, sign_len):
    """ Calculate the boot image's total length
    """

    total_siz = filesizes["resource_start"]
    if "resource/pubkey" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pubkey)"]
    if "encryption/iv" in filesizes:
        total_siz = total_siz + filesizes["round(encryption/iv)"]
    if "resource/private2" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private2)"]
    elif "resource/private" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private)"]
    total_siz = round_up(total_siz, 256)
    total_siz = total_siz + sign_len
    return total_siz


def check_loader_run_in_dram(cfg):
    """
        Legacy code, will be removed in later's version
    """

    if "loader" not in cfg:
        return False
    if "run in dram" in cfg["loader"]:
        if cfg["loader"]["run in dram"].upper() == "FALSE":
            return False
    return True


def _ensure_encryption_iv(cfg: dict) -> None:
    """Auto-generate IV/nonce file when encryption is configured but none provided.

    Generates a random IV/nonce whose size depends on the algorithm:
      - ChaCha20: 12-byte nonce
      - AES/SM4:  16-byte IV

    Saves it to ``<datadir>/<imgname>.iv.bin`` and injects the path back
    into ``cfg["encryption"]["iv"]`` so that all downstream functions
    (size calculation, resource packing, encryption) pick it up.
    """
    if "encryption" not in cfg:
        return
    if "iv" in cfg["encryption"]:
        return
    datadir = cfg.get("datadir", ".")
    outname = cfg.get("_outname", "auto")
    algo = cfg["encryption"].get("algo", "")
    _, iv_size = _ENC_KEY_IV_MAP.get(algo, (16, 16))
    iv_name = os.path.splitext(outname)[0] + ".iv.bin"
    iv_path = os.path.join(datadir, iv_name)
    iv_data = os.urandom(iv_size)
    with open(iv_path, "wb") as f:
        f.write(iv_data)
    cfg["encryption"]["iv"] = iv_path
    if VERBOSE:
        print("\tAuto-generated IV/nonce ({}B): {}".format(iv_size, iv_path))


def aic_boot_get_encryption_key(cfg, ssk_derived=False):
    algo = cfg["encryption"].get("algo", "aes-128-cbc")
    key_size, iv_size = _ENC_KEY_IV_MAP.get(algo, (16, 16))

    fpath = get_file_path(cfg["encryption"]["key"], cfg["keydir"])
    if fpath is None:
        fpath = get_file_path(cfg["encryption"]["key"], cfg["datadir"])
    if fpath is None:
        print('Please provide key file')
        sys.exit(1)
    keydata = None
    ivdata = None
    if ssk_derived:
        # Use SSK(Symmetric Secure Key) derived AES key to encrypt it
        #
        #                SSK(128bit)
        #                 | (key)
        #                 v
        #  KM(128bit) -> AES -> HSK(128bit, in Secure SRAM)
        #                        | (key)
        #                        v
        #      SPL plaintext -> AES -> SPL ciphertext

        # Only encrypt loader content, if loader not exist, don't do it
        try:
            with open(fpath, "rb") as f:
                key_material = b"0123456789abcdef"
                symmetric_secure_key = f.read(16)
                cipher = AES.new(symmetric_secure_key, AES.MODE_ECB)
                hardware_secure_key = cipher.encrypt(key_material)
                keydata = hardware_secure_key
        except IOError:
            print('Failed to open symmetric secure key file')
            sys.exit(1)
    else:
        try:
            with open(fpath, "rb") as f:
                keydata = f.read(key_size)
        except IOError:
            print('Failed to open key file')
            sys.exit(1)

    if iv_size > 0:
        fpath = get_file_path(cfg["encryption"]["iv"], cfg["keydir"])
        if fpath is None:
            fpath = get_file_path(cfg["encryption"]["iv"], cfg["datadir"])
        try:
            with open(fpath, "rb") as f:
                ivdata = f.read(iv_size)
        except IOError:
            print('Failed to open iv file')
            sys.exit(1)

    return keydata, ivdata


def aic_boot_get_loader_bytes(cfg, filesizes):
    """ Read the loader's binaray data, and perform encryption if it is needed.

        Legacy code, will be removed in later's version
    """

    loader_size = 0
    header_size = 256
    rawbytes = bytearray(0)
    if check_loader_run_in_dram(cfg):
        # No loader in first aicimg
        # Record the information to generate header and resource bytes
        filesizes["resource_start"] = header_size + loader_size
        return rawbytes

    if "round(loader/file)" in filesizes:
        loader_size = filesizes["round(loader/file)"]
        try:
            fpath = get_file_path(cfg["loader"]["file"], cfg["datadir"])
            with open(fpath, "rb") as f:
                rawbytes = f.read(loader_size)
        except IOError:
            print("Failed to open loader file: {}".format(fpath))
            sys.exit(1)

        if len(rawbytes) == 0:
            print("Read loader data failed.")
            sys.exit(1)
        if len(rawbytes) < loader_size:
            rawbytes = rawbytes + bytearray(loader_size - len(rawbytes))

    # Record the information to generate header and resource bytes
    filesizes["resource_start"] = header_size + loader_size

    # Use SSK(Symmetric Secure Key) derived AES key to encrypt it
    #
    #                SSK(128bit)
    #                 | (key)
    #                 v
    #  KM(128bit) -> AES -> HSK(128bit, in Secure SRAM)
    #                        | (key)
    #                        v
    #      SPL plaintext -> AES -> SPL ciphertext
    if "encryption" in cfg and loader_size > 0:
        keydata, ivdata = aic_boot_get_encryption_key(cfg, True)
        if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-ecb":
            cipher = AES.new(keydata, AES.MODE_CBC, ivdata)
            enc_bytes = cipher.encrypt(rawbytes)
            return enc_bytes
        elif "encryption" in cfg and cfg["encryption"]["algo"] == "sm4-ecb":
            # Only encrypt loader content, if loader not exist, don't do it
            cipher = SM4.CryptSM4()
            cipher.set_key(keydata, SM4.SM4_ENCRYPT)
            enc_bytes = cipher.crypt_ecb(rawbytes)
            return enc_bytes[0:len(rawbytes)]
    else:
        return rawbytes


def aic_boot_get_loader_for_ext(cfg, filesizes):
    """ Read the loader's binaray data, and perform encryption if it is needed.

        Legacy code, will be removed in later's version
    """

    loader_size = 0
    rawbytes = bytearray(0)
    if "round(loader/file)" in filesizes:
        loader_size = filesizes["round(loader/file)"]
        try:
            fpath = get_file_path(cfg["loader"]["file"], cfg["datadir"])
            with open(fpath, "rb") as f:
                rawbytes = f.read(loader_size)
        except IOError:
            print("Failed to open loader file: {}".format(fpath))
            sys.exit(1)

        if len(rawbytes) == 0:
            print("Read loader data failed.")
            sys.exit(1)
        if len(rawbytes) < loader_size:
            rawbytes = rawbytes + bytearray(loader_size - len(rawbytes))

    header_size = 256
    # Record the information to generate header and resource bytes
    filesizes["resource_start"] = header_size + loader_size

    # Use SSK(Symmetric Secure Key) derived AES key to encrypt it
    #
    #                SSK(128bit)
    #                 | (key)
    #                 v
    #  KM(128bit) -> AES -> HSK(128bit, in Secure SRAM)
    #                        | (key)
    #                        v
    #      SPL plaintext -> AES -> SPL ciphertext
    if "encryption" in cfg and loader_size > 0:
        keydata, ivdata = aic_boot_get_encryption_key(cfg, True)
        cipher = AES.new(keydata, AES.MODE_CBC, ivdata)
        enc_bytes = cipher.encrypt(rawbytes)
        return enc_bytes
    else:
        return rawbytes


def aic_boot_get_loader_bytes_v2(cfg, filesizes):
    """ Read the loader's binaray data, and perform encryption if it is needed.
    """

    loader_size = 0
    header_size = 256
    rawbytes = bytearray(0)

    if "round(loader/file)" in filesizes:
        loader_size = filesizes["round(loader/file)"]
        try:
            fpath = get_file_path(cfg["loader"]["file"], cfg["datadir"])
            with open(fpath, "rb") as f:
                rawbytes = f.read(loader_size)
        except IOError:
            print("Failed to open loader file: {}".format(fpath))
            sys.exit(1)

        if len(rawbytes) == 0:
            print("Read loader data failed.")
            sys.exit(1)
        if len(rawbytes) < loader_size:
            rawbytes = rawbytes + bytearray(loader_size - len(rawbytes))

    # Record the information to generate header and resource bytes
    filesizes["resource_start"] = header_size + loader_size

    if "encryption" in cfg and loader_size > 0:
        derive = aic_boot_use_ssk_derived_key(cfg)
        keydata, ivdata = aic_boot_get_encryption_key(cfg, derive)
        if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-cbc":
            cipher = AES.new(keydata, AES.MODE_CBC, ivdata)
            enc_bytes = cipher.encrypt(rawbytes)
            return enc_bytes
        elif "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-ctr":
            init_val = int.from_bytes(ivdata, 'big')
            ctr = Counter.new(128, initial_value=init_val)
            cipher = AES.new(keydata, AES.MODE_CTR, counter=ctr)
            enc_bytes = cipher.encrypt(rawbytes)
            return enc_bytes
        elif "encryption" in cfg and cfg["encryption"]["algo"] == "sm4-cbc":
            cipher = SM4.CryptSM4()
            cipher.set_key(keydata, SM4.SM4_ENCRYPT)
            enc_bytes = cipher.crypt_cbc(ivdata, rawbytes)
            return enc_bytes[0:len(rawbytes)]
        elif "encryption" in cfg and cfg["encryption"]["algo"] == "sm4-ecb":
            # Only encrypt loader content, if loader not exist, don't do it
            cipher = SM4.CryptSM4()
            cipher.set_key(keydata, SM4.SM4_ENCRYPT)
            enc_bytes = cipher.crypt_ecb(rawbytes)
            return enc_bytes[0:len(rawbytes)]
        elif "encryption" in cfg and cfg["encryption"]["algo"] == "chacha20":
            cipher = ChaCha20.new(key=keydata, nonce=ivdata)
            enc_bytes = cipher.encrypt(rawbytes)
            return enc_bytes
        else:
            print('Unknown encryption ALGO')
            sys.exit(1)
    else:
        return rawbytes


def aic_boot_private2_data_sign(cfg, privdata):
    if "signature" in cfg and cfg["signature"]["algo"] == "hmac,sha256":
        newdata = privdata[0:4] + bytearray(4)
        newdata = newdata + privdata[8:12] + int_to_uint32_bytes(len(privdata)) + privdata[16:]
        signature = aic_boot_gen_hmac_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
    elif "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        newdata = privdata[0:4] + bytearray(4)
        newdata = newdata + privdata[8:12] + int_to_uint32_bytes(len(privdata)) + privdata[16:]
        signature = aic_boot_gen_rsa_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
    elif "signature" in cfg and cfg["signature"]["algo"] == "sm2":
        newdata = privdata[0:4] + bytearray(4)
        newdata = newdata + privdata[8:12] + int_to_uint32_bytes(len(privdata)) + privdata[16:]
        signature = aic_boot_gen_sm2_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
    else:
        signed_bytes = privdata
    return signed_bytes


def aic_boot_pbp2_enc_and_sign(cfg, pbp_data):
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
    out_bytes = pbp_data
    prog_data = pbp_data[32:]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-cbc":
        keydata, ivdata = aic_boot_get_encryption_key(cfg, False)
        cipher = AES.new(keydata, AES.MODE_CBC, ivdata)
        enc_bytes = cipher.encrypt(prog_data)
        prog_data = enc_bytes
        # Update checksum first for no signature case
        newdata = pbp_data[0:4] + bytearray(4) + pbp_data[8:32] + enc_bytes
        cksum = aic_calc_checksum(newdata, len(newdata))
        out_bytes = pbp_data[0:4] + int_to_uint32_bytes(cksum) + newdata[8:]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-ctr":
        keydata, ivdata = aic_boot_get_encryption_key(cfg, False)
        init_val = int.from_bytes(ivdata, 'big')
        ctr = Counter.new(128, initial_value=init_val)
        cipher = AES.new(keydata, AES.MODE_CTR, counter=ctr)
        enc_bytes = cipher.encrypt(prog_data)
        prog_data = enc_bytes
        # Update checksum first for no signature case
        newdata = pbp_data[0:4] + bytearray(4) + pbp_data[8:32] + enc_bytes
        cksum = aic_calc_checksum(newdata, len(newdata))
        out_bytes = pbp_data[0:4] + int_to_uint32_bytes(cksum) + newdata[8:]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "sm4-cbc":
        keydata, ivdata = aic_boot_get_encryption_key(cfg, False)
        cipher = SM4.CryptSM4()
        cipher.set_key(keydata, SM4.SM4_ENCRYPT)
        enc_bytes = cipher.crypt_cbc(ivdata, prog_data)
        prog_data = enc_bytes[0:len(prog_data)]
        # Update checksum first for no signature case
        newdata = pbp_data[0:4] + bytearray(4) + pbp_data[8:32] + enc_bytes
        cksum = aic_calc_checksum(newdata, len(newdata))
        out_bytes = pbp_data[0:4] + int_to_uint32_bytes(cksum) + newdata[8:]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "chacha20":
        keydata, ivdata = aic_boot_get_encryption_key(cfg, False)
        cipher = ChaCha20.new(key=keydata, nonce=ivdata)
        enc_bytes = cipher.encrypt(prog_data)
        prog_data = enc_bytes
        # Update checksum first for no signature case
        newdata = pbp_data[0:4] + bytearray(4) + pbp_data[8:32] + enc_bytes
        cksum = aic_calc_checksum(newdata, len(newdata))
        out_bytes = pbp_data[0:4] + int_to_uint32_bytes(cksum) + newdata[8:]
    if "signature" in cfg and cfg["signature"]["algo"] == "hmac,sha256":
        newdata = pbp_data[0:4] + bytearray(4)
        newdata = newdata + pbp_data[8:12] + int_to_uint32_bytes(len(pbp_data)) + pbp_data[16:32]
        newdata = newdata + prog_data
        signature = aic_boot_gen_hmac_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
        out_bytes = signed_bytes
    elif "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        newdata = pbp_data[0:4] + bytearray(4)
        newdata = newdata + pbp_data[8:12] + int_to_uint32_bytes(len(pbp_data)) + pbp_data[16:32]
        newdata = newdata + prog_data
        signature = aic_boot_gen_rsa_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
        out_bytes = signed_bytes
    elif "signature" in cfg and cfg["signature"]["algo"] == "sm2":
        newdata = pbp_data[0:4] + bytearray(4)
        newdata = newdata + pbp_data[8:12] + int_to_uint32_bytes(len(pbp_data)) + pbp_data[16:32]
        newdata = newdata + prog_data
        signature = aic_boot_gen_sm2_signature_bytes(cfg, newdata)
        signed_bytes = newdata + signature
        out_bytes = signed_bytes
    return out_bytes


def aic_boot_get_resource_bytes(cfg, filesizes):
    """ Pack all resource data into boot image's resource section
    """
    resbytes = bytearray(0)
    if "resource/pbp2" in filesizes:
        pbp_size = filesizes["round(resource/pbp2)"]
        try:
            fpath = get_file_path(cfg["resource"]["pbp2"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pbp_data = f.read(pbp_size)
        except IOError:
            print('Failed to open pbp file')
            sys.exit(1)
        paddata = pbp_data + bytearray(pbp_size - len(pbp_data))
        pbp_data = aic_boot_pbp2_enc_and_sign(cfg, paddata)
        filesizes["round(resource/pbp2)"] = len(pbp_data)
        resbytes = resbytes + pbp_data
    elif "resource/pbp" in filesizes:
        pbp_size = filesizes["round(resource/pbp)"]
        try:
            fpath = get_file_path(cfg["resource"]["pbp"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pbp_data = f.read(pbp_size)
        except IOError:
            print('Failed to open pbp file')
            sys.exit(1)
        resbytes = resbytes + pbp_data + bytearray(pbp_size - len(pbp_data))
    if "resource/private2" in filesizes:
        priv_size = filesizes["round(resource/private2)"]
        try:
            fpath = get_file_path(cfg["resource"]["private2"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)

        paddata = privdata + bytearray(priv_size - len(privdata))
        privdata = aic_boot_private2_data_sign(cfg, paddata)
        filesizes["round(resource/private2)"] = len(privdata)
        resbytes = resbytes + privdata
    elif "resource/private" in filesizes:
        priv_size = filesizes["round(resource/private)"]
        try:
            fpath = get_file_path(cfg["resource"]["private"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)
        resbytes = resbytes + privdata + bytearray(priv_size - len(privdata))
    if "resource/pubkey" in filesizes:
        pubkey_size = filesizes["round(resource/pubkey)"]
        try:
            fpath = get_file_path(cfg["resource"]["pubkey"], cfg["keydir"])
            if fpath is None:
                fpath = get_file_path(cfg["resource"]["pubkey"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pkdata = f.read(pubkey_size)
        except IOError:
            print('Failed to open pubkey file')
            sys.exit(1)
        # Add padding to make it alignment
        resbytes = resbytes + pkdata + bytearray(pubkey_size - len(pkdata))
    if "encryption/iv" in filesizes:
        iv_size = filesizes["round(encryption/iv)"]
        try:
            fpath = get_file_path(cfg["encryption"]["iv"], cfg["keydir"])
            if fpath is None:
                fpath = get_file_path(cfg["encryption"]["iv"], cfg["datadir"])
            with open(fpath, "rb") as f:
                ivdata = f.read(iv_size)
        except IOError:
            print('Failed to open iv file')
            sys.exit(1)
        resbytes = resbytes + ivdata + bytearray(iv_size - len(ivdata))
    if len(resbytes) > 0:
        res_size = round_up(len(resbytes), 256)
        if len(resbytes) != res_size:
            resbytes = resbytes + bytearray(res_size - len(resbytes))
    return resbytes


def aic_boot_get_resource_for_ext(cfg, filesizes):
    """ Pack all resource data into boot image's resource section
    """

    resbytes = bytearray(0)
    if "resource/private2" in filesizes:
        priv_size = filesizes["round(resource/private2)"]
        try:
            fpath = get_file_path(cfg["resource"]["private2"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)

        paddata = privdata + bytearray(priv_size - len(privdata))
        privdata = aic_boot_private2_data_sign(cfg, paddata)
        filesizes["round(resource/private2)"] = len(privdata)
        resbytes = resbytes + privdata
    if "resource/private" in filesizes:
        priv_size = filesizes["round(resource/private)"]
        try:
            fpath = get_file_path(cfg["resource"]["private"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)
        resbytes = resbytes + privdata + bytearray(priv_size - len(privdata))
    if "resource/pubkey" in filesizes:
        pubkey_size = filesizes["round(resource/pubkey)"]
        try:
            fpath = get_file_path(cfg["resource"]["pubkey"], cfg["keydir"])
            if fpath is None:
                fpath = get_file_path(cfg["resource"]["pubkey"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pkdata = f.read(pubkey_size)
        except IOError:
            print('Failed to open pubkey file')
            sys.exit(1)
        # Add padding to make it alignment
        resbytes = resbytes + pkdata + bytearray(pubkey_size - len(pkdata))
    if "encryption/iv" in filesizes:
        iv_size = filesizes["round(encryption/iv)"]
        try:
            fpath = get_file_path(cfg["encryption"]["iv"], cfg["keydir"])
            if fpath is None:
                fpath = get_file_path(cfg["encryption"]["iv"], cfg["datadir"])
            with open(fpath, "rb") as f:
                ivdata = f.read(iv_size)
        except IOError:
            print('Failed to open iv file')
            sys.exit(1)
        resbytes = resbytes + ivdata + bytearray(iv_size - len(ivdata))
    if len(resbytes) > 0:
        res_size = round_up(len(resbytes), 256)
        if len(resbytes) != res_size:
            resbytes = resbytes + bytearray(res_size - len(resbytes))
    return resbytes


def aic_boot_checksum(bootimg):
    length = len(bootimg)
    offset = 0
    total = 0
    while offset < length:
        val = int.from_bytes(bootimg[offset: offset + 4], byteorder='little', signed=False)
        total = total + val
        offset = offset + 4
    return (~total) & 0xFFFFFFFF


def aic_calc_checksum(start, size):
    offset = 0
    total = 0
    while offset < size:
        val = int.from_bytes(start[offset: offset + 4], byteorder='little', signed=False)
        total = total + val
        offset = offset + 4
    return (~total) & 0xFFFFFFFF


def aic_boot_add_header(h, n):
    return h + n.to_bytes(4, byteorder='little', signed=False)


def aic_boot_with_ext_loader(cfg):
    if "with_ext" in cfg and cfg["with_ext"].upper() == "TRUE":
        return True
    return False


def aic_boot_use_ssk_derived_key(cfg):
    # A special case
    if "ssk_derived_key" in cfg and cfg["ssk_derived_key"].upper() == "TRUE":
        return True
    return False


def _get_sign_info(cfg, img_len, default_len=16):
    """ Get signature algorithm info from config

    Args:
        cfg: Configuration dict
        img_len: Total image length
        default_len: Default sign length when no signature is configured

    Returns:
        Tuple of (sign_algo, sign_length, sign_offset)
    """
    if "signature" in cfg:
        algo = cfg["signature"]["algo"]
        if algo in _SIGN_ALGO_MAP:
            info = _SIGN_ALGO_MAP[algo]
            return info["algo"], info["len"], img_len - info["len"]
    return 0, default_len, img_len - default_len


def _get_enc_info_v1(cfg, next_res_offset, filesizes, loader_length):
    """ Get encryption algorithm info for v1 header (legacy)

    In v1, IV data is only valid when loader_length != 0 for cbc/ctr modes.

    Args:
        cfg: Configuration dict
        next_res_offset: Next available resource offset
        filesizes: Dict of file sizes
        loader_length: Loader binary length

    Returns:
        Tuple of (enc_algo, iv_data_offset, iv_data_length, next_res_offset)
    """
    enc_algo = 0
    iv_data_offset = 0
    iv_data_length = 0
    if "encryption" in cfg:
        algo = cfg["encryption"]["algo"]
        if loader_length != 0 and algo in ("aes-128-cbc", "aes-128-ctr", "sm4-cbc", "chacha20"):
            enc_algo = _ENC_ALGO_MAP[algo]
            iv_data_offset = next_res_offset
            _, iv_size = _ENC_KEY_IV_MAP.get(algo, (16, 16))
            iv_data_length = iv_size
            next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
        elif algo == "sm4-ecb":
            enc_algo = _ENC_ALGO_MAP[algo]
    return enc_algo, iv_data_offset, iv_data_length, next_res_offset


def _get_enc_info_v2(cfg, next_res_offset, filesizes):
    """ Get encryption algorithm info for v2 header

    In v2, IV data is always valid for cbc/ctr modes regardless of loader.

    Args:
        cfg: Configuration dict
        next_res_offset: Next available resource offset
        filesizes: Dict of file sizes

    Returns:
        Tuple of (enc_algo, iv_data_offset, iv_data_length, next_res_offset)
    """
    enc_algo = 0
    iv_data_offset = 0
    iv_data_length = 0
    if "encryption" in cfg:
        algo = cfg["encryption"]["algo"]
        if algo in ("aes-128-cbc", "aes-128-ctr", "sm4-cbc", "chacha20"):
            enc_algo = _ENC_ALGO_MAP[algo]
            iv_data_offset = next_res_offset
            _, iv_size = _ENC_KEY_IV_MAP.get(algo, (16, 16))
            iv_data_length = iv_size
            next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
        elif algo == "sm4-ecb":
            enc_algo = _ENC_ALGO_MAP[algo]
    return enc_algo, iv_data_offset, iv_data_length, next_res_offset


def _calc_resource_offsets(cfg, filesizes):
    """ Calculate resource data offsets and lengths for header fields

    Computes the offset and length of pbp, private, and pubkey resource data
    based on the resource_start offset and file sizes.

    Args:
        cfg: Configuration dict
        filesizes: Dict of file sizes including "resource_start"

    Returns:
        Tuple of (pbp_data_offset, pbp_data_length, priv_data_offset,
                  priv_data_length, sign_key_offset, sign_key_length,
                  next_res_offset)
    """
    next_res_offset = filesizes["resource_start"]
    # Calculate PBP data offset and length
    pbp_data_offset = 0
    pbp_data_length = 0
    if "resource" in cfg:
        res = cfg["resource"]
        for key, round_key in [("pbp2", "round(resource/pbp2)"), ("pbp", "round(resource/pbp)")]:
            if key in res:
                pbp_data_offset = next_res_offset
                if key == "pbp2":
                    pbp_data_length = filesizes[round_key]
                else:
                    pbp_data_length = filesizes["resource/pbp"]
                next_res_offset = pbp_data_offset + filesizes[round_key]
                break
    # Calculate private data offset and length
    priv_data_offset = 0
    priv_data_length = 0
    if "resource" in cfg:
        res = cfg["resource"]
        for key, round_key in [("private2", "round(resource/private2)"),
                               ("private", "round(resource/private)")]:
            if key in res:
                priv_data_offset = next_res_offset
                if key == "private2":
                    priv_data_length = filesizes[round_key]
                else:
                    priv_data_length = filesizes["resource/private"]
                next_res_offset = priv_data_offset + filesizes[round_key]
                break
    # Calculate sign key offset and length
    sign_key_offset = 0
    sign_key_length = 0
    if "resource" in cfg and "pubkey" in cfg["resource"]:
        sign_key_offset = next_res_offset
        # Set the length value equal to real size
        sign_key_length = filesizes["resource/pubkey"]
        # Calculate offset use the size after alignment
        next_res_offset = sign_key_offset + filesizes["round(resource/pubkey)"]
    return (pbp_data_offset, pbp_data_length, priv_data_offset,
            priv_data_length, sign_key_offset, sign_key_length, next_res_offset)


def _gen_header_bytes(magic, fields):
    """ Generate 256-byte header from magic string and field values

    Args:
        magic: Magic string (e.g. "AIC ")
        fields: List of integer field values to append

    Returns:
        256-byte header as bytearray
    """
    header_bytes = magic.encode(encoding="utf-8")
    for f in fields:
        header_bytes = aic_boot_add_header(header_bytes, f)
    header_bytes = header_bytes + bytearray(256 - len(header_bytes))
    return header_bytes


def _get_loader_addr(cfg):
    """ Get loader load address and entry point from config

    If only load address is provided, entry point defaults to load_address + 256.
    If only entry point is provided, load address defaults to entry_point - 256.

    Args:
        cfg: Configuration dict

    Returns:
        Tuple of (load_address, entry_point)
    """
    load_address = 0
    entry_point = 0
    if "loader" in cfg:
        if "load address" in cfg["loader"]:
            if cfg["loader"]["load address"] == "CONFIG_AIC_BOOTLOADER_LOAD_BASE":
                print("Error: Please compile the bootloader first.")
                sys.exit(1)
            load_address = int(cfg["loader"]["load address"], 16)
        if "entry point" in cfg["loader"]:
            if cfg["loader"]["entry point"] == "CONFIG_AIC_BOOTLOADER_TEXT_BASE":
                print("Error: Please compile the bootloader first.")
                sys.exit(1)
            entry_point = int(cfg["loader"]["entry point"], 16)
        if "load address" in cfg["loader"] and "entry point" not in cfg["loader"]:
            entry_point = load_address + 256
        if "load address" not in cfg["loader"] and "entry point" in cfg["loader"]:
            load_address = entry_point - 256
    return load_address, entry_point


def aic_boot_gen_header_bytes(cfg, filesizes):
    """ Generate header bytes

        Legacy code, will be removed in later's version
    """
    magic = "AIC "
    checksum = 0
    header_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        header_ver = int(cfg["head_ver"], 16)

    sign_len = _get_sign_len(cfg)
    default_sign_len = 16 if sign_len == 0 else sign_len
    img_len = aic_boot_calc_image_length(filesizes, default_sign_len)
    fw_ver = 0
    if "anti-rollback counter" in cfg:
        fw_ver = cfg["anti-rollback counter"]

    loader_length = 0
    if "loader/file" in filesizes:
        loader_length = filesizes["loader/file"]

    loader_ext_offset = 0
    if check_loader_run_in_dram(cfg):
        loader_length = 0
        loader_ext_offset = round_up(img_len, META_ALIGNED_SIZE)

    load_address, entry_point = _get_loader_addr(cfg)
    sign_algo, sign_length, sign_offset = _get_sign_info(cfg, img_len, 16)
    (pbp_data_offset, pbp_data_length, priv_data_offset,
     priv_data_length, sign_key_offset, sign_key_length,
     next_res_offset) = _calc_resource_offsets(cfg, filesizes)
    enc_algo, iv_data_offset, iv_data_length, _ = _get_enc_info_v1(
        cfg, next_res_offset, filesizes, loader_length)

    return _gen_header_bytes(magic, [
        checksum, header_ver, img_len, fw_ver, loader_length,
        load_address, entry_point, sign_algo, enc_algo,
        sign_offset, sign_length, sign_key_offset, sign_key_length,
        iv_data_offset, iv_data_length, priv_data_offset, priv_data_length,
        pbp_data_offset, pbp_data_length, loader_ext_offset])


def aic_boot_gen_header_for_ext(cfg, filesizes):
    """ Generate header bytes

        Legacy code, will be removed in later's version
    """
    # Prepare header information
    magic = "AIC "
    checksum = 0
    header_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        header_ver = int(cfg["head_ver"], 16)

    if "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        img_len = aic_boot_calc_image_length_for_ext(filesizes, 256)
    elif "signature" in cfg and cfg["signature"]["algo"] == "sm2":
        img_len = aic_boot_calc_image_length_for_ext(filesizes, 64)
    elif "signature" in cfg and cfg["signature"]["algo"] == "hmac,sha256":
        img_len = aic_boot_calc_image_length_for_ext(filesizes, 32)
    else:
        img_len = aic_boot_calc_image_length_for_ext(filesizes, 16)
    fw_ver = 0

    loader_length = 0
    if "loader/file" in filesizes:
        loader_length = filesizes["loader/file"]

    loader_ext_offset = 0

    load_address = 0
    entry_point = 0
    if "loader" in cfg:
        if "load address ext" in cfg["loader"]:
            load_address = int(cfg["loader"]["load address ext"], 16)
        else:
            if cfg["loader"]["load address"] == "CONFIG_AIC_BOOTLOADER_LOAD_BASE":
                print("Error: Please compile the bootloader first.")
                sys.exit(1)
            load_address = int(cfg["loader"]["load address"], 16)
        if "entry point ext" in cfg["loader"]:
            entry_point = int(cfg["loader"]["entry point ext"], 16)
        else:
            if cfg["loader"]["entry point"] == "CONFIG_AIC_BOOTLOADER_TEXT_BASE":
                print("Error: Please compile the bootloader first.")
                sys.exit(1)
            entry_point = int(cfg["loader"]["entry point"], 16)
    sign_algo = 0
    sign_offset = 0
    sign_length = 0
    sign_key_offset = 0
    sign_key_length = 0
    next_res_offset = filesizes["resource_start"]
    priv_data_offset = 0
    priv_data_length = 0
    if "resource" in cfg and "private" in cfg["resource"]:
        priv_data_offset = next_res_offset
        priv_data_length = filesizes["resource/private"]
        next_res_offset = priv_data_offset + filesizes["round(resource/private)"]
    if "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        sign_algo = 1
        sign_length = 256
        sign_offset = img_len - sign_length
    elif "signature" in cfg and cfg["signature"]["algo"] == "hmac,sha256":
        sign_algo = 3
        sign_length = 32
        sign_offset = img_len - sign_length
    else:
        # Append md5 result to the end
        sign_algo = 0
        sign_length = 16
        sign_offset = img_len - sign_length

    if "resource" in cfg and "pubkey" in cfg["resource"]:
        sign_key_offset = next_res_offset
        # Set the length value equal to real size
        sign_key_length = filesizes["resource/pubkey"]
        # Calculate offset use the size after alignment
        next_res_offset = sign_key_offset + filesizes["round(resource/pubkey)"]
    enc_algo = 0
    iv_data_offset = 0
    iv_data_length = 0
    if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-cbc":
        enc_algo = 1
        iv_data_offset = next_res_offset
        iv_data_length = 16
        next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-ctr":
        enc_algo = 4
        iv_data_offset = next_res_offset
        iv_data_length = 16
        next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "sm4-cbc":
        enc_algo = 3
        iv_data_offset = next_res_offset
        iv_data_length = 16
        next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
    if "encryption" in cfg and cfg["encryption"]["algo"] == "chacha20":
        enc_algo = 5
        iv_data_offset = next_res_offset
        iv_data_length = 12
        next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
    pbp_data_offset = 0
    pbp_data_length = 0
    # Generate header bytes
    header_bytes = magic.encode(encoding="utf-8")
    header_bytes = aic_boot_add_header(header_bytes, checksum)
    header_bytes = aic_boot_add_header(header_bytes, header_ver)
    header_bytes = aic_boot_add_header(header_bytes, img_len)
    header_bytes = aic_boot_add_header(header_bytes, fw_ver)
    header_bytes = aic_boot_add_header(header_bytes, loader_length)
    header_bytes = aic_boot_add_header(header_bytes, load_address)
    header_bytes = aic_boot_add_header(header_bytes, entry_point)
    header_bytes = aic_boot_add_header(header_bytes, sign_algo)
    header_bytes = aic_boot_add_header(header_bytes, enc_algo)
    header_bytes = aic_boot_add_header(header_bytes, sign_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_length)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_length)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_length)
    header_bytes = aic_boot_add_header(header_bytes, loader_ext_offset)
    header_bytes = header_bytes + bytearray(256 - len(header_bytes))
    return header_bytes


def aic_boot_gen_header_bytes_v2(cfg, filesizes):
    """ Generate header bytes
    """
    magic = "AIC "
    checksum = 0
    header_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        header_ver = int(cfg["head_ver"], 16)

    cksum_algo = cfg.get("checksum-algo", "md5").strip().lower()
    cksum_aux_len = _CKSUM_ALGO_MAP.get(cksum_algo, 0)
    sign_len = _get_sign_len(cfg)
    default_sign_len = sign_len if sign_len else cksum_aux_len
    img_len = aic_boot_calc_image_length(filesizes, default_sign_len)
    fw_ver = 0
    if "anti-rollback counter" in cfg:
        fw_ver = cfg["anti-rollback counter"]

    loader_length = 0
    if "loader/file" in filesizes:
        loader_length = filesizes["loader/file"]

    loader_ext_offset = 0
    if aic_boot_with_ext_loader(cfg):
        loader_length = 0
        loader_ext_offset = round_up(img_len, META_ALIGNED_SIZE)

    load_address, entry_point = _get_loader_addr(cfg)
    sign_algo, sign_length, sign_offset = _get_sign_info(cfg, img_len, cksum_aux_len)
    (pbp_data_offset, pbp_data_length, priv_data_offset,
     priv_data_length, sign_key_offset, sign_key_length,
     next_res_offset) = _calc_resource_offsets(cfg, filesizes)
    enc_algo, iv_data_offset, iv_data_length, _ = _get_enc_info_v2(
        cfg, next_res_offset, filesizes)

    return _gen_header_bytes(magic, [
        checksum, header_ver, img_len, fw_ver, loader_length,
        load_address, entry_point, sign_algo, enc_algo,
        sign_offset, sign_length, sign_key_offset, sign_key_length,
        iv_data_offset, iv_data_length, priv_data_offset, priv_data_length,
        pbp_data_offset, pbp_data_length, loader_ext_offset])


def aic_boot_gen_rsa_signature_bytes(cfg, bootimg):
    """ Generate RSASSA-PKCS1-v1.5 Signature with SHA-256
    """
    if "privkey" not in cfg["signature"]:
        print("RSA Private key is not exist.")
        sys.exit(1)
    try:
        fpath = get_file_path(cfg["signature"]["privkey"], cfg["keydir"])
        if fpath is None:
            fpath = get_file_path(cfg["signature"]["privkey"], cfg["datadir"])
        with open(fpath, 'rb') as frsa:
            rsakey = RSA.importKey(frsa.read())
    except IOError:
        print("Failed to open file: " + cfg["signature"]["privkey"])
        sys.exit(1)
    # Check if it is private key
    if rsakey.has_private() is False:
        print("Should to use RSA private key to sign")
        sys.exit(1)
    keysize = max(1, math.ceil(rsakey.n.bit_length() / 8))
    if keysize != 256:
        print("Only RSA 2048 is supported, please input RSA 2048 Private Key.")
        sys.exit(1)
    # Calculate SHA-256 hash
    sha256 = SHA256.new()
    sha256.update(bootimg)
    # Encrypt the hash, and using RSASSA-PKCS1-V1.5 Padding
    signer = PKCS1_v1_5.new(rsakey)
    sign_bytes = signer.sign(sha256)
    return sign_bytes


def aic_boot_gen_hmac_signature_bytes(cfg, bootimg):
    """ Generate ArtInChip specified HMAC-SHA256 authentication code calculate flow for
        firmware component:

        1. Use privkey to perform AES-128-ECB encrypt first 64 bytes, the output will be used as
           HMAC key
        2. Use the calculated HMAC key to calculate firmware component's authentication code.
    """
    if "privkey" not in cfg["signature"]:
        print("HMAC Private key is not exist.")
        sys.exit(1)
    try:
        if os.path.exists(cfg["keydir"] + cfg["signature"]["privkey"]):
            fpath = cfg["keydir"] + cfg["signature"]["privkey"]
        else:
            fpath = cfg["datadir"] + cfg["signature"]["privkey"]
        with open(fpath, 'rb') as fkey:
            privkey = fkey.read()
    except IOError:
        print("Failed to open file: " + cfg["signature"]["privkey"])
        sys.exit(1)
    # Check if it is private key
    if len(privkey) != 16:
        print("Should provide 16 bytes private key to sign")
        sys.exit(1)
    if "key-derive" in cfg["signature"] and cfg["signature"]["key-derive"] == "aes-128-ctr":
        iv = b"0123456789ABCDEF"
        init_val = int.from_bytes(iv, 'big')
        ctr = Counter.new(128, initial_value=init_val)
        cipher = AES.new(privkey, AES.MODE_CTR, counter=ctr)
    else:
        cipher = AES.new(privkey, AES.MODE_ECB)
    keyseed = bootimg[0:64]
    hmackey = cipher.encrypt(keyseed)
    # print(f"\n\nlen: {len(bootimg)}")
    # print(bootimg[:32])
    # print(f"hmackey:{hmackey.hex()}")
    hmac = HMAC.new(hmackey, digestmod=SHA256)
    hmac.update(bootimg)
    sign_bytes = hmac.digest()
    # print(f"sign_bytes:{sign_bytes.hex()}")
    return sign_bytes


def aic_boot_gen_signature_bytes(cfg, bootimg):
    if "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        return aic_boot_gen_rsa_signature_bytes(cfg, bootimg)
    elif "signature" in cfg and cfg["signature"]["algo"] == "hmac,sha256":
        return aic_boot_gen_hmac_signature_bytes(cfg, bootimg)
    elif "signature" in cfg and cfg["signature"]["algo"] == "sm2":
        return aic_boot_gen_sm2_signature_bytes(cfg, bootimg)
    else:
        print("Not support signature algorithm")
        sys.exit(1)


def aic_boot_gen_img_md5_bytes(cfg, bootimg):
    """ Calculate MD5 of image to make brom verify image faster
    """
    # Calculate MD5 hash
    md5 = MD5.new()
    md5.update(bootimg)
    md5_bytes = md5.digest()
    return md5_bytes


def aic_boot_check_params(cfg):
    if ("encryption" in cfg
            and cfg["encryption"]["algo"] not in _ENC_ALGO_MAP):
        print("Only support {} encryption".format(
            "/".join(_ENC_ALGO_MAP.keys())))
        return False
    # if "signature" in cfg and cfg["signature"]["algo"] != "rsa,2048":
    #     print("Only support rsa,2048 signature")
    #     return False
    # if "loader" not in cfg or "load address" not in cfg["loader"]:
    #     print("load address is not set")
    #     return False
    # if "loader" not in cfg or "entry point" not in cfg["loader"]:
    #     print("entry point is not set")
    #     return False
    return True


def get_sm2_key_pair(derfile):
    pk = None
    pr = None
    try:
        with open(derfile, 'rb') as fsm2:
            asn1 = asn1crypto.core.load(fsm2.read())
            # asn1.debug()
            asn1._parse_children()
            pr = asn1.children[1][4]
            value = asn1.children[3][4][2:]
            if value[0]:
                pk = value
            else:
                pk = value[1:]
    except IOError:
        print('Failed to open file: ' + derfile)
        sys.exit(1)
    priv_key_hex = binascii.hexlify(pr).decode('utf-8')
    pub_key_hex = binascii.hexlify(pk).decode('utf-8')
    return (priv_key_hex, pub_key_hex)


def aic_boot_gen_sm2_signature_bytes(cfg, bootimg):
    """ Generate SM2 Signature with SM3
    """
    if "privkey" not in cfg["signature"]:
        print("SM2 Private key is not exist.")
        sys.exit(1)
    if os.path.exists(cfg["keydir"] + cfg["signature"]["privkey"]):
        fpath = cfg["keydir"] + cfg["signature"]["privkey"]
    else:
        fpath = cfg["datadir"] + cfg["signature"]["privkey"]
    (pr, pk) = get_sm2_key_pair(fpath)
    sm2_crypt = SM2.CryptSM2(public_key=pk, private_key=pr)
    sm3_str = SM3.sm3_hash(bytearray(bootimg))
    sm3_bin = binascii.unhexlify(sm3_str)
    # Debug
    # random_str = sm3_str
    random_str = func.random_hex(sm2_crypt.para_len)
    # random_str = 'fadc36018fcc350ffd1783553d6ede3790eda384cd61eeb923a52f51bb2762ea'
    sign_str = sm2_crypt.sign(sm3_bin, random_str)
    sign_bytes = binascii.unhexlify(sign_str)
    return sign_bytes


def aic_boot_gen_img_sm3_bytes(cfg, bootimg):
    """ Calculate SM3 of image to make brom verify image faster
    """
    # Calculate SM3 hash

    sm3_str = SM3.sm3_hash(bytearray(bootimg))
    # print(sm3_str)
    # with open('check_sm3.bin', 'wb') as f:
    #     f.write(bootimg)
    sm3_bytes = binascii.unhexlify(sm3_str)
    return sm3_bytes


def aic_boot_create_image(cfg, keydir, datadir):
    """ Create AIC format Boot Image for Boot ROM

        Legacy code, will be removed in later's version
    """
    if aic_boot_check_params(cfg) is False:
        sys.exit(1)
    _ensure_encryption_iv(cfg)
    filesizes = aic_boot_get_resource_file_size(cfg, keydir, datadir)

    loader_bytes = aic_boot_get_loader_bytes(cfg, filesizes)
    resource_bytes = bytearray(0)
    if "resource" in cfg or "encryption" in cfg:
        resource_bytes = aic_boot_get_resource_bytes(cfg, filesizes)
    header_bytes = aic_boot_gen_header_bytes(cfg, filesizes)
    bootimg = header_bytes + loader_bytes + resource_bytes

    head_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        head_ver = int(cfg["head_ver"], 16)
    if "signature" in cfg:
        signature_bytes = aic_boot_gen_signature_bytes(cfg, bootimg)
        bootimg = bootimg + signature_bytes
        return bootimg

    # Secure boot is not enabled, always add md5 result to the end
    md5_bytes = aic_boot_gen_img_md5_bytes(cfg, bootimg[8:])
    bootimg = bootimg + md5_bytes
    # Calculate checksum.
    # When MD5 is disabled, checksum will be checked by BROM.
    cs = aic_boot_checksum(bootimg)
    cs_bytes = cs.to_bytes(4, byteorder='little', signed=False)
    bootimg = bootimg[0:4] + cs_bytes + bootimg[8:]
    # Verify the checksum value
    cs = aic_boot_checksum(bootimg)
    if cs != 0:
        print("Checksum is error: {}".format(cs))
        sys.exit(1)
    return bootimg


def aic_boot_create_ext_image(cfg, keydir, datadir):
    """ Create AIC format Boot Image for Boot ROM

        Legacy code, will be removed in later's version
    """

    _ensure_encryption_iv(cfg)
    filesizes = aic_boot_get_resource_file_size(cfg, keydir, datadir)
    loader_bytes = aic_boot_get_loader_for_ext(cfg, filesizes)
    resource_bytes = bytearray(0)
    if "resource" in cfg:
        resource_bytes = aic_boot_get_resource_for_ext(cfg, filesizes)
    header_bytes = aic_boot_gen_header_for_ext(cfg, filesizes)
    bootimg = header_bytes + loader_bytes + resource_bytes

    head_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        head_ver = int(cfg["head_ver"], 16)
    if "signature" in cfg:
        signature_bytes = aic_boot_gen_signature_bytes(cfg, bootimg)
        bootimg = bootimg + signature_bytes
        return bootimg

    # Secure boot is not enabled, always add md5 result to the end
    md5_bytes = aic_boot_gen_img_md5_bytes(cfg, bootimg[8:])
    bootimg = bootimg + md5_bytes
    # Calculate checksum.
    # When MD5 is disabled, checksum will be checked by BROM.
    cs = aic_boot_checksum(bootimg)
    cs_bytes = cs.to_bytes(4, byteorder='little', signed=False)
    bootimg = bootimg[0:4] + cs_bytes + bootimg[8:]
    # Verify the checksum value
    cs = aic_boot_checksum(bootimg)
    if cs != 0:
        print("Checksum is error: {}".format(cs))
        sys.exit(1)
    return bootimg


def aic_boot_create_image_v2(cfg, keydir, datadir):
    """ Create AIC format Boot Image for Boot ROM
    """
    if aic_boot_check_params(cfg) is False:
        sys.exit(1)
    _ensure_encryption_iv(cfg)
    filesizes = aic_boot_get_resource_file_size(cfg, keydir, datadir)

    loader_bytes = aic_boot_get_loader_bytes_v2(cfg, filesizes)
    resource_bytes = bytearray(0)
    if "resource" in cfg or "encryption" in cfg:
        resource_bytes = aic_boot_get_resource_bytes(cfg, filesizes)
    header_bytes = aic_boot_gen_header_bytes_v2(cfg, filesizes)
    bootimg = header_bytes + loader_bytes + resource_bytes

    head_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        head_ver = int(cfg["head_ver"], 16)
    if "signature" in cfg:
        signature_bytes = aic_boot_gen_signature_bytes(cfg, bootimg)
        bootimg = bootimg + signature_bytes
        if aic_boot_with_ext_loader(cfg):
            padlen = round_up(len(bootimg), META_ALIGNED_SIZE) - len(bootimg)
            if padlen > 0:
                bootimg += bytearray(padlen)
        return bootimg

    # Default is MD5
    cksum_algo = "md5"
    if "checksum-algo" in cfg:
        cksum_algo = cfg["checksum-algo"].strip().lower()
    if cksum_algo == "md5":
        # Secure boot is not enabled, always add md5 result to the end
        md5_bytes = aic_boot_gen_img_md5_bytes(cfg, bootimg[8:])
        bootimg = bootimg + md5_bytes
    if cksum_algo == "sm3":
        sm3_bytes = aic_boot_gen_img_sm3_bytes(cfg, bootimg[8:])
        bootimg = bootimg + sm3_bytes
    if aic_boot_with_ext_loader(cfg):
        padlen = round_up(len(bootimg), META_ALIGNED_SIZE) - len(bootimg)
        if padlen > 0:
            bootimg += bytearray(padlen)
    # Always Calculate simple checksum.
    cs = aic_boot_checksum(bootimg)
    cs_bytes = cs.to_bytes(4, byteorder='little', signed=False)
    bootimg = bootimg[0:4] + cs_bytes + bootimg[8:]
    # Verify the checksum value
    cs = aic_boot_checksum(bootimg)
    if cs != 0:
        print("Checksum is error: {}".format(cs))
        sys.exit(1)
    return bootimg


def itb_create_image(itsname, itbname, keydir, dtbname, script_dir):
    from itbimage import build_itb
    build_itb(
        its_file=itsname,
        output_file=itbname,
        external_data=True,
        block_align=0x800,
        keydir=keydir,
        keydest=dtbname,
        require_keys=(dtbname is not None),
    )


def spienc_create_image(imgcfg, script_dir):

    keypath = get_file_path(imgcfg["key"], imgcfg["keydir"])
    if keypath is None:
        keypath = get_file_path(imgcfg["key"], imgcfg["datadir"])

    mkcmd = os.path.join(script_dir, "spienc")
    if os.path.exists(mkcmd) is False:
        mkcmd = "spienc"
    if sys.platform == "win32":
        mkcmd += ".exe"
    cmd = [mkcmd]
    cmd.append("--key")
    cmd.append("{}".format(keypath))
    if "nonce" in imgcfg:
        noncepath = get_file_path(imgcfg["nonce"], imgcfg["keydir"])
        if noncepath is None:
            noncepath = get_file_path(imgcfg["nonce"], imgcfg["datadir"])
        cmd.append("--nonce")
        cmd.append("{}".format(noncepath))
    if "tweak" in imgcfg:
        cmd.append("--tweak")
        cmd.append("{}".format(imgcfg["tweak"]))
    cmd.append("--addr")
    cmd.append("{}".format(imgcfg["address"]))
    cmd.append("--input")
    cmd.append("{}".format(imgcfg["input"]))
    cmd.append("--output")
    cmd.append("{}".format(imgcfg["output"]))
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        print(ret.stdout.decode("utf-8"))
        sys.exit(1)


def spienctool_key_encrypt(imgcfg, script_dir):
    mkcmd = os.path.join(script_dir, "spienctool")
    if os.path.exists(mkcmd) is False:
        mkcmd = "spienctool"
    if sys.platform == "win32":
        mkcmd += ".exe"
    cmd = [mkcmd]
    cmd.append(imgcfg["operation"])
    cmd.append("--seedver")
    cmd.append(imgcfg["seedver"])
    cmd.append("--spienckey")
    spienckeypath = get_file_path(imgcfg["spienckey"], imgcfg["keydir"])
    if spienckeypath is None:
        spienckeypath = get_file_path(imgcfg["spienckey"], imgcfg["datadir"])
    cmd.append("{}".format(spienckeypath))
    cmd.append("--key")
    cmd.append("{}".format(imgcfg["input"]))
    cmd.append("--out")
    cmd.append("{}".format(imgcfg["output"]))
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        print(ret.stdout.decode("utf-8"))
        sys.exit(1)


def data_crypt_create_image(imgcfg, script_dir):
    keypath = get_file_path(imgcfg["key"], imgcfg["keydir"])
    if keypath is None:
        keypath = get_file_path(imgcfg["key"], imgcfg["datadir"])

    mkcmd = os.path.join(script_dir, "firmware_security_encrypt")
    if os.path.exists(mkcmd) is False:
        mkcmd = "firmware_security_encrypt"
    if sys.platform == "win32":
        mkcmd += ".exe"
    cmd = [mkcmd]
    cmd.append("--key")
    cmd.append("{}".format(keypath))
    if "nonce" in imgcfg:
        noncepath = get_file_path(imgcfg["nonce"], imgcfg["keydir"])
        if noncepath is None:
            noncepath = get_file_path(imgcfg["nonce"], imgcfg["datadir"])
        cmd.append("--nonce")
        cmd.append("{}".format(noncepath))
    if "tweak" in imgcfg:
        cmd.append("--tweak")
        cmd.append("{}".format(imgcfg["tweak"]))
    cmd.append("--infile")
    cmd.append("{}".format(imgcfg["input"]))
    cmd.append("--outfile")
    cmd.append("{}".format(imgcfg["output"]))
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        print(ret.stdout.decode("utf-8"))
        sys.exit(1)


def concatenate_create_image(outname, flist, datadir):
    with open(outname, "wb") as fout:
        for fn in flist:
            fpath = get_file_path(fn, datadir)
            if fpath is None:
                print("Error, {} is not found.".format(fn))
                sys.exit(1)
            fin = open(fpath, "rb")
            data = fin.read()
            fout.write(data)
            fin.close()


def img_gen_fw_file_name(cfg):
    # Image file name format:
    # <platform>_<product>_v<version>_c<anti-rollback counter>.img
    img_file_name = cfg["image"]["info"]["platform"]
    img_file_name += "_"
    img_file_name += cfg["image"]["info"]["product"]
    img_file_name += "_v"
    img_file_name += cfg["image"]["info"]["version"]
    if "anti-rollback" in cfg["image"]["info"]:
        img_file_name += "_c"
        img_file_name += cfg["image"]["info"]["anti-rollback"]
    img_file_name += ".img"
    return img_file_name.replace(" ", "_")


def calc_crc32(fname, size):
    """Calculate crc32 for a file
    Args:
        fname: file path
    """
    hash = 0
    step = 16 * 1024
    if size > 0:
        step = size

    if os.path.exists(fname) is False:
        return 0

    with open(fname, 'rb') as fp:
        while True:
            s = fp.read(step)
            if not s:
                break
            hash = zlib.crc32(s, hash)
            if size > 0:
                # only need to calc first 'size' byte
                break
    return hash & 0xffffffff


def size_str_to_int(size_str):
    if "k" in size_str or "K" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024)
    if "m" in size_str or "M" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024 * 1024)
    if "g" in size_str or "G" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024 * 1024 * 1024)
    if "0x" in size_str or "0X" in size_str:
        return int(size_str, 16)
    return 0


def str_to_int(s):
    """ String to number
    """
    if s.startswith('0x') or s.startswith('0X'):
        return int(s, 16)
    else:
        return int(s)


def bytes_to_int(s):
    """ String to number
    """
    s = str_from_nbytes(s)
    if s.startswith('0x') or s.startswith('0X'):
        return int(s, 16)
    else:
        return int(s)


def str_to_nbytes(s, n):
    """ String to n bytes
    """
    ba = bytearray(s, encoding="utf-8")
    nzero = n - len(ba)
    if nzero > 0:
        ba.extend([0] * nzero)
    return bytes(ba)


def str_from_nbytes(s):
    """ String from n bytes
    """
    return str(s, encoding='utf-8')


def val_to_int(val):
    """ Maybe int, or hex string
    """
    if isinstance(val, int):
        return val
    if isinstance(val, str):
        return int(val, 16)
    return 0


def int_to_uint32_bytes(n):
    """ Int value to uint32 bytes
    """
    return n.to_bytes(4, byteorder='little', signed=False)


def int_to_uint8_bytes(n):
    """ Int value to uint8 bytes
    """
    return n.to_bytes(1, byteorder='little', signed=False)


def int_to_uint16_bytes(n):
    """ Int value to uint8 bytes
    """
    return n.to_bytes(2, byteorder='little', signed=False)


def int_from_uint32_bytes(s):
    """ Int value from uint32 bytes
    """
    return int.from_bytes(s, byteorder='little', signed=False)


def gen_bytes(n, length):
    """ gen len uint8 bytes
    """
    return bytearray([n] * length)


def _extract_media_info(cfg):
    """Extract and normalize media info from config (support both old and new format)

    Args:
        cfg: Configuration dictionary

    Returns:
        list: List of dicts with keys: "name", "type", "controller"

    Old format: {"type": "spi-nor", "device_id": 0}
    New format: {"name": ["spi-nor0", "spi-nand1"], "controller": [0, 1]}
    """
    media_cfg = cfg["image"]["info"]["media"]
    media_list = []

    if "type" in media_cfg:
        # Old format: type and device_id
        media_type = media_cfg["type"]
        device_id = media_cfg.get("device_id", 0)

        # Normalize to list (handle both string and list formats)
        if isinstance(media_type, str):
            media_types_list = [media_type]
        else:
            media_types_list = media_type

        if isinstance(device_id, (int, str)):
            device_ids_list = [device_id]
        else:
            device_ids_list = device_id

        # Ensure device_ids_list has same length as media_types_list
        while len(device_ids_list) < len(media_types_list):
            device_ids_list.append(0)

        for mt, ctrl_id in zip(media_types_list, device_ids_list):
            # Convert ctrl_id to int if it's a string
            if isinstance(ctrl_id, str):
                ctrl_id = int(ctrl_id)

            media_list.append({
                "name": mt,
                "type": mt,
                "controller": ctrl_id
            })
    else:
        # New format: name and controller list
        device_names = media_cfg["name"]
        controllers = media_cfg.get("controller", [])

        for idx, device_name in enumerate(device_names):
            # Get media type from device config (if no "type" key, device_name is the type)
            media_type = cfg[device_name].get("type", device_name)
            ctrl_id = controllers[idx] if idx < len(controllers) else 0

            media_list.append({
                "name": device_name,
                "type": media_type,
                "controller": ctrl_id
            })

    return media_list


"""
struct artinchip_fw_hdr{
    char magic[8];
    char platform[64];
    char product[64];
    char version[64];
    char media_type[64];
    u32  media_dev_id;
    u8   nand_array_org[64];/* NAND Array Organization */
    u32  meta_offset; /* Meta Area start offset */
    u32  meta_size;   /* Meta Area size */
    u32  file_offset; /* File data Area start offset */
    u32  file_size;   /* File data Area size */
};
"""


def img_write_fw_header(imgfile, cfg, meta_area_size, file_area_size):
    """ Generate Firmware image's header data
    Args:
        cfg: Dict from JSON
        meta_area_size: size of meta data area
        file_area_size: size of file data area
    """
    array_org_len = 64
    nand_array_org = ""
    if "array_organization" in cfg["image"]["info"]["media"]:
        array_orgval = cfg["image"]["info"]["media"]["array_organization"]
        if not isinstance(array_orgval, list):
            print("Error, nand array organization should be a list.")
            return -1
        param_str = ""
        for item in array_orgval:
            param_str += "P={},B={};".format(item["page"].upper(), item["block"].upper())
        param_str = param_str[0:-1]
        nand_array_org = param_str

    # Get media info (support both old and new format)
    media_list = _extract_media_info(cfg)

    magic = "AIC.FW"
    platform = str(cfg["image"]["info"]["platform"])
    product = str(cfg["image"]["info"]["product"])
    version = str(cfg["image"]["info"]["version"])

    # media_type: join multiple types with semicolon
    media_types = [str(m["type"]) for m in media_list]
    media_type = ";".join(media_types)

    # media_dev_id: pack controller IDs as bytes (max 4 devices)
    dev_ids = [m["controller"] for m in media_list[:4]]  # Limit to 4 devices
    while len(dev_ids) < 4:
        dev_ids.append(0)  # Pad with 0
    media_dev_id = bytes(dev_ids)  # 4 bytes

    meta_offset = DATA_ALIGNED_SIZE
    meta_size = meta_area_size
    file_offset = DATA_ALIGNED_SIZE + meta_area_size
    file_size = file_area_size

    buff = str_to_nbytes("AIC.FW", 8)
    buff = buff + str_to_nbytes(platform, 64)
    buff = buff + str_to_nbytes(product, 64)
    buff = buff + str_to_nbytes(version, 64)
    buff = buff + str_to_nbytes(media_type, 64)
    buff = buff + media_dev_id  # Already 4 bytes
    buff = buff + str_to_nbytes(nand_array_org, 64)
    buff = buff + int_to_uint32_bytes(meta_offset)
    buff = buff + int_to_uint32_bytes(meta_size)
    buff = buff + int_to_uint32_bytes(file_offset)
    buff = buff + int_to_uint32_bytes(file_size)
    imgfile.seek(0, 0)
    imgfile.write(buff)
    imgfile.flush()
    if VERBOSE:
        print("\tImage header is generated.")
    return 0


"""
struct artinchip_fwc_meta {
    char magic[8];
    char name[64];
    char partition[32];
    u32  part_offset;
    u32  part_size;
    char dev_name[20];
    u32  dev_id;
    u32  offset_in_img;
    u32  size_in_img;
    u32  crc;
    u32  ram;
    char attr[64]
    char filename[64]
};
"""


def img_gen_fwc_meta(name, dev_name, dev_id, part, part_offset, part_size,
                     offset_in_img, size_in_img, crc, ram, attr, filename):
    """ Generate Firmware component's meta data
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    buff = str_to_nbytes("META", 8)
    buff = buff + str_to_nbytes(name, 64)
    buff = buff + str_to_nbytes(part, 32)
    buff = buff + int_to_uint32_bytes(part_offset)
    buff = buff + int_to_uint32_bytes(part_size)
    buff = buff + str_to_nbytes(dev_name, 20)
    buff = buff + int_to_uint32_bytes(dev_id)
    buff = buff + int_to_uint32_bytes(offset_in_img)
    buff = buff + int_to_uint32_bytes(size_in_img)
    buff = buff + int_to_uint32_bytes(crc)
    buff = buff + int_to_uint32_bytes(ram)
    buff = buff + str_to_nbytes(attr, 64)
    buff = buff + str_to_nbytes(filename, 64)

    if VERBOSE:
        print("\t\tMeta for {:<25} offset in file {:<10} size in file {} ({})".format(
              name, hex(offset_in_img), hex(size_in_img), size_in_img))
    return buff


PAGE_TABLE_MAX_ENTRY = 101

"""
struct nand_page_table_head {
    char magic[4]; /* AICP: AIC Page table */
    u32 entry_cnt;
    u16 page_size;
    u8 pages_per_block;
    u8 blocks; /* Block number in SPI NAND */
    u8 planes; /* Plane number in SPI NAND: Max 2 in market avaialbe devices */
    u8 pad[7]; /* Padding it to fit size 20 bytes */
};

struct nand_page_table_entry {
    u32 pageaddr1;
    u32 pageaddr2;
    u32 checksum2;
    u32 reserved;
    u32 checksum1;
};

struct nand_page_table {
    struct nand_page_table_head head;
    struct nand_page_table_entry entry[PAGE_TABLE_MAX_ENTRY];
};
"""


def img_gen_page_table(binfile, cfg, datadir):
    """ Generate page table data
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    page_size = 0
    page_cnt = 64
    planes = 1

    if "array_organization" in cfg["image"]["info"]["media"]:
        orglist = cfg["image"]["info"]["media"]["array_organization"]
        for item in orglist:
            page_size = int(re.sub(r"[^0-9]", "", item["page"]))
            block_size = int(re.sub(r"[^0-9]", "", item["block"]))

    if "planes" in cfg["image"]["info"]["media"]:
        val = cfg["image"]["info"]["media"]["planes"]
        if isinstance(val, str):
            planes = int(val)
        else:
            planes = val
        if planes > 2:
            print("Error, planes should not be greater than 2.")
            sys.exit(1)

    spl_file = cfg["image"]["target"]["spl"]["file"]
    filesize = round_up(cfg["image"]["target"]["spl"]["filesize"], DATA_ALIGNED_SIZE)
    page_per_blk = block_size // page_size
    page_cnt = filesize // (page_size * 1024)
    if (page_cnt + 1 > (2 * PAGE_TABLE_MAX_ENTRY)):
        print("SPL too large, more than 400K.")
        sys.exit(1)

    path = get_file_path(spl_file, datadir)
    if path is None:
        sys.exit(1)

    step = page_size * 1024

    entry_page = page_cnt + 1
    buff = str_to_nbytes("AICP", 4)
    buff = buff + int_to_uint32_bytes(entry_page)
    buff = buff + int_to_uint16_bytes(page_size * 1024)
    buff = buff + int_to_uint8_bytes(page_per_blk)
    buff = buff + int_to_uint8_bytes(0xFF)
    buff = buff + int_to_uint8_bytes(planes)
    buff = buff + gen_bytes(0xFF, 7)

    with open(path, "rb") as fwcfile:
        pageaddr1 = 0
        pageaddr2 = PAGE_TABLE_MAX_ENTRY

        if (pageaddr1 < PAGE_TABLE_MAX_ENTRY):
            buff = buff + int_to_uint32_bytes(pageaddr1)
        else:
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

        if (pageaddr2 < (2 * PAGE_TABLE_MAX_ENTRY) and pageaddr2 <= (page_cnt + 1)):
            offset2 = (pageaddr2 - 1) * (page_size * 1024)
            fwcfile.seek(offset2, 0)
            bindata = fwcfile.read(step)
            checksum2 = aic_calc_checksum(bindata, page_size * 1024)

            buff = buff + int_to_uint32_bytes(pageaddr2)
            buff = buff + int_to_uint32_bytes(checksum2)
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
        else:
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

        if (pageaddr1 < PAGE_TABLE_MAX_ENTRY):
            buff = buff + int_to_uint32_bytes(0)
        else:
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

        for i in range(1, PAGE_TABLE_MAX_ENTRY):
            pageaddr1 = i
            pageaddr2 = PAGE_TABLE_MAX_ENTRY + i

            if (pageaddr1 < PAGE_TABLE_MAX_ENTRY and pageaddr1 <= (page_cnt + 1)):
                buff = buff + int_to_uint32_bytes(pageaddr1)
            else:
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

            if (pageaddr2 < (2 * PAGE_TABLE_MAX_ENTRY) and pageaddr2 <= (page_cnt + 1)):
                offset2 = (pageaddr2 - 1) * (page_size * 1024)
                fwcfile.seek(offset2, 0)
                bindata = fwcfile.read(step)
                checksum2 = aic_calc_checksum(bindata, page_size * 1024)

                buff = buff + int_to_uint32_bytes(pageaddr2)
                buff = buff + int_to_uint32_bytes(checksum2)
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
            else:
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

            if (pageaddr1 < PAGE_TABLE_MAX_ENTRY):
                offset1 = (pageaddr1 - 1) * (page_size * 1024)
                fwcfile.seek(offset1, 0)
                bindata = fwcfile.read(step)
                checksum1 = aic_calc_checksum(bindata, page_size * 1024)

                buff = buff + int_to_uint32_bytes(checksum1)
            else:
                buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

    buff = buff + gen_bytes(0xFF, page_size * 1024 - len(buff))
    checksum = aic_calc_checksum(buff, page_size * 1024)
    buff = buff[0:36] + int_to_uint32_bytes(checksum) + buff[40:]

    binfile.seek(0, 0)
    binfile.write(buff)
    binfile.flush()

    if VERBOSE:
        print("\tPage table is generated.")

    return 0


def check_partition_exist(table, partval):
    if isinstance(partval, list):
        for item in partval:
            if item.find(":") > 0: # UBI Volume
                parts = item.split(":")
                part = parts[0]
                vol = parts[1]
                if part not in table:
                    print("{} not in table {}".format(part, table))
                    return False

                if "ubi" in table[part]:
                    if vol not in table[part]["ubi"]:
                        print("{} not in ubi {}".format(vol, table[part]["ubi"]))
                        return False
                elif "nftl" in table[part]:
                    if vol not in table[part]["nftl"]:
                        print("{} not in nftl {}".format(vol, table[part]["nftl"]))
                        return False
                else:
                    print("{} not in {}".format(vol, table[part]))
                    return False
            else:
                if item not in table:
                    print("{} not in table {}".format(partval, table))
                    return False
    else:
        if partval not in table:
            print("{} not in table {}".format(partval, table))
            return False
    return True


def img_write_fwc_meta_section(imgfile, cfg, sect, meta_off, file_off, datadir):
    fwcset = cfg["image"][sect]

    # Build partition lookup table across all devices for validation
    media_list = _extract_media_info(cfg)
    all_partitions = {}
    ctrl_map = {}
    for media in media_list:
        device_name = media["name"]
        ctrl_map[device_name] = media["controller"]
        if device_name in cfg and "partitions" in cfg[device_name]:
            all_partitions.update(cfg[device_name]["partitions"])

    for fwc in fwcset:
        file_size = fwcset[fwc]["filesize"]
        part_offset = 0
        part_size = 0
        dev_name = "ram"
        dev_id = 0
        if sect == "target":
            if "part_size" not in fwcset[fwc]:
                print("There is no partition for component '{}', please remove it.".format(fwc))
                continue
            part_size = fwcset[fwc]["part_size"]
            part_offset = fwcset[fwc]["part_offset"]
            dev_name = fwcset[fwc]["media_type"]
            dev_id = ctrl_map.get(fwcset[fwc]["device_name"], 0)
            if file_size > part_size:
                part_name = fwcset[fwc]["part"]
                if isinstance(part_name, list):
                    part_name = ";".join(part_name)
                print("Target '{}': file '{}' size {} exceeds partition '{}' size {}".format(
                    fwc, fwcset[fwc]["file"], hex(file_size),
                    part_name, hex(part_size)))
                return (-1, -1)
        if file_size <= 0:
            continue
        imgfile.seek(meta_off, 0)
        path = str(datadir + fwcset[fwc]["file"])
        crc = calc_crc32(path, 0)
        if "ram" in fwcset[fwc]:
            ram = int(fwcset[fwc]["ram"], 16)
        else:
            ram = 0xFFFFFFFF
        attrval = fwcset[fwc]["attr"]
        if isinstance(attrval, list):
            attr = str(";".join(attrval))
        else:
            attr = str(attrval)
        attr = attr.replace(' ', '')
        name = str("image." + sect + "." + fwc)

        if "part" in fwcset[fwc]:
            partval = fwcset[fwc]["part"]
            if check_partition_exist(all_partitions, partval) is False:
                print("Partition {} not exist".format(partval))
                return (-1, -1)
            if isinstance(partval, list):
                part = str(";".join(partval))
            else:
                part = str(partval)
        else:
            part = ""
        file_name = fwcset[fwc]["file"]
        meta = img_gen_fwc_meta(name, dev_name, dev_id, part, part_offset, part_size,
                                file_off, file_size, crc, ram, attr, file_name)
        imgfile.write(meta)
        fwcset[fwc]["meta_off"] = meta_off
        fwcset[fwc]["file_off"] = file_off
        # Update for next item
        meta_off += META_ALIGNED_SIZE
        file_size = round_up(file_size, DATA_ALIGNED_SIZE)
        file_off += file_size
    return (meta_off, file_off)


def img_write_fwc_meta_to_imgfile(imgfile, cfg, meta_start, file_start, datadir):
    """ Generate and write FW component's meta data
    Args:
        imgfile: Image file handle
        cfg: Dict from JSON
        meta_start: meta data area start offset
        file_start: file data area start offset
        datadir: working directory
    """
    meta_offset = meta_start
    file_offset = file_start
    if VERBOSE:
        print("\tMeta data for image components:")
    # 1, FWC of updater
    meta_offset, file_offset = img_write_fwc_meta_section(imgfile, cfg, "updater",
                                                          meta_offset, file_offset,
                                                          datadir)
    if meta_offset < 0:
        return -1
    # 2, Image Info(The same with image header)
    imgfile.seek(meta_offset, 0)
    img_fn = datadir + img_gen_fw_file_name(cfg)
    crc = calc_crc32(img_fn, DATA_ALIGNED_SIZE)
    # Image info is the image header, start from 0
    info_offset = 0
    info_size = DATA_ALIGNED_SIZE
    part = ""
    name = "image.info"
    ram = 0xFFFFFFFF
    attr = "required"
    file_name = "info.bin"
    meta = img_gen_fwc_meta(name, "ram", 0, part, 0, 0, info_offset, info_size, crc, ram, attr, file_name)
    imgfile.write(meta)
    # Only meta offset increase
    meta_offset += META_ALIGNED_SIZE
    # 3, FWC of target
    meta_offset, file_offset = img_write_fwc_meta_section(imgfile, cfg, "target",
                                                          meta_offset, file_offset,
                                                          datadir)
    if meta_offset < 0:
        return -1
    imgfile.flush()
    return 0


def img_write_fwc_file_to_imgfile(imgfile, cfg, file_start, datadir):
    """ Write FW component's file data
    Args:
        imgfile: Image file handle
        cfg: Dict from JSON
        file_start: file data area start offset
        datadir: working directory
    """
    file_offset = file_start
    if VERBOSE:
        print("\tPacking file data:")
    for section in ["updater", "target"]:
        fwcset = cfg["image"][section]
        for fwc in fwcset:
            path = get_file_path(fwcset[fwc]["file"], datadir)
            if path is None:
                continue
            if VERBOSE:
                print("\t\t" + os.path.split(path)[1])
            # Read fwc file content, and write to image file
            imgfile.seek(file_offset, 0)
            step = 16 * 1024
            with open(path, "rb") as fwcfile:
                while True:
                    bindata = fwcfile.read(step)
                    if not bindata:
                        break
                    imgfile.write(bindata)
            # Update for next file
            filesize = fwcset[fwc]["filesize"]
            filesize = round_up(filesize, DATA_ALIGNED_SIZE)
            file_offset += filesize
    imgfile.flush()
    return 0


BIN_FILE_MAX_SIZE = 300 * 1024 * 1024


def _calc_block_info(media_type, part_offset, part_size, filesize, block_size):
    """ Calculate block info (start_block, last_block, used_block, total_block) for different
        media types

    Args:
        media_type: Media type string ("spi-nand", "spi-nor", "mmc")
        part_offset: Partition offset in bytes
        part_size: Partition size in bytes
        filesize: File data size in bytes
        block_size: Block size in bytes (only used for spi-nand)

    Returns:
        Tuple of (start_block, last_block, used_block, total_block, block_size)
    """
    last_block = 0
    total_block = 0
    if media_type == "spi-nand":
        start_block = part_offset // block_size // 1024
        used_block = filesize // block_size // 1024
        if filesize % (block_size * 1024) != 0:
            used_block += 1
        total_block = part_size // block_size // 1024
        last_block = start_block + total_block - 1
    elif media_type == "spi-nor":
        block_size = 64
        start_block = part_offset // block_size // 1024
        used_block = filesize // block_size // 1024
        if filesize % (block_size * 1024) != 0:
            used_block += 1
        last_block = start_block + used_block - 1
    elif media_type == "mmc":
        block_size = 512
        start_block = part_offset // block_size
        used_block = filesize // block_size
        if filesize % (block_size * 1024) != 0:
            used_block += 1
        last_block = start_block + used_block - 1
    return start_block, last_block, used_block, total_block, block_size


def _fill_padding(binfile, size, fill_byte=0xFF, step=1024 * 1024):
    """ Write padding bytes to binfile in chunks

    Args:
        binfile: Binary file handle to write to
        size: Total padding size in bytes
        fill_byte: Fill byte value (default 0xFF)
        step: Chunk size for each write operation
    """
    while size > 0:
        write_size = min(size, step)
        binfile.write(gen_bytes(fill_byte, write_size))
        size -= write_size


def img_write_fwc_file_to_binfile(binfile, cfg, datadir, device_name=None,
                                  part_table_file=None):
    """ Write FW component's file data
    Args:
        binfile: Image bin file handle
        cfg: Dict from JSON
        datadir: working directory
        device_name: Only write file data of this device, default is the first device
        part_table_file: Output part table file path, default is burner dir's part table
    """
    page_size = 0
    block_size = 0

    if "array_organization" in cfg["image"]["info"]["media"]:
        orglist = cfg["image"]["info"]["media"]["array_organization"]
        for item in orglist:
            page_size = int(re.sub(r"[^0-9]", "", item["page"]))
            block_size = int(re.sub(r"[^0-9]", "", item["block"]))

    media_list = _extract_media_info(cfg)
    if device_name is None:
        device_name = media_list[0]["name"]
    media_size = size_str_to_int(cfg[device_name]["size"])

    if (media_size > BIN_FILE_MAX_SIZE):
        media_size = BIN_FILE_MAX_SIZE

    # Fill the entire bin file with 0xFF padding up to media size
    _fill_padding(binfile, int(media_size) - int(binfile.tell()))

    page_table_size = page_size * 1024

    buff = bytes()

    if VERBOSE:
        print("\tPacking file data:")
    for section in ["target"]:
        fwcset = cfg["image"][section]
        for fwc in fwcset:
            path = get_file_path(fwcset[fwc]["file"], datadir)
            if path is None:
                continue
            if path.find(".ubifs") != -1:
                path = path.replace(".ubifs", ".ubi")
                if os.path.exists(path) is False:
                    print("File {} is not exist".format(path))
                    continue
            if VERBOSE:
                print("\t\t" + os.path.split(path)[1])

            fwc_device_name = fwcset[fwc].get("device_name", media_list[0]["name"])
            media_type = fwcset[fwc].get("media_type", media_list[0]["type"])

            if fwc_device_name != device_name:
                if VERBOSE:
                    msg = "\t\tSkipping " + os.path.split(path)[1]
                    msg += " (belongs to " + fwc_device_name + ")"
                    print(msg)
                continue

            part_offset = fwcset[fwc]["part_offset"]
            part_size = fwcset[fwc]["part_size"]
            part_name = fwcset[fwc]["part"][0]
            filesize = round_up(os.stat(path).st_size, DATA_ALIGNED_SIZE)

            # SPL on SPI-NAND needs extra page table space
            is_spl_nand = fwc == "spl" and media_type == "spi-nand"
            if is_spl_nand:
                filesize += page_table_size

            start_block, last_block, used_block, total_block, block_size = \
                _calc_block_info(media_type, part_offset, part_size, filesize, block_size)

            if media_type == "spi-nand":
                if (total_block - used_block) <= (total_block // 50):
                    print("\t\tPart {} reserved blocks are less than 2%, \
                            bad blocks may cause burning failures".format(part_name))
                if last_block < (start_block + used_block - 1):
                    print("\t\tFile {} exceeds the part {} size".format(path, part_name))
                    sys.exit(1)

            buff = buff + int_to_uint32_bytes(start_block)
            buff = buff + int_to_uint32_bytes(last_block)
            buff = buff + int_to_uint32_bytes(used_block)
            buff = buff + int_to_uint32_bytes(0xFFFFFFFF)

            if is_spl_nand:
                part_offset += page_table_size
                filesize -= page_table_size
            binfile.seek(part_offset, 0)
            step = 1024 * 1024
            with open(path, "rb") as fwcfile:
                while True:
                    bindata = fwcfile.read(step)
                    if not bindata:
                        break
                    binfile.write(bindata)
            binfile.seek(part_offset + filesize, 0)

            if is_spl_nand:
                filesize += page_table_size

            if (part_size - filesize < 0):
                print("file {} size({}) exceeds {} partition size({})".format(fwcset[fwc]["file"],
                                                                              filesize,
                                                                              part_name,
                                                                              part_size))
                sys.exit(1)

            _fill_padding(binfile, part_size - filesize)
    binfile.flush()

    buff = buff + gen_bytes(0xFF, 16)
    if part_table_file is None:
        part_table_file = datadir + "burner/" + cfg["image"]["part_table"]
    with open(part_table_file, "wb") as partfile:
        partfile.write(buff)
        partfile.flush()

    return 0


def img_get_fwc_file_size(cfg, datadir):
    """ Scan directory and get Firmware component's file size, update to cfg
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    for section in ["updater", "target"]:
        fwcset = cfg["image"][section]
        for fwc in fwcset:
            path = get_file_path(fwcset[fwc]["file"], datadir)
            if path is None:
                attr = fwcset[fwc]["attr"]
                if "required" in attr:
                    print("Error, file {} is not exist".format(fwcset[fwc]["file"]))
                    return -1
                else:
                    # FWC file is not exist, but it is not necessary
                    fwcset[fwc]["filesize"] = 0
                    continue
            statinfo = os.stat(path)
            fwcset[fwc]["filesize"] = statinfo.st_size
    return 0


def img_get_part_size(cfg, datadir):
    part_name = ""
    part_size = 0
    part_offs = 0
    total_siz = 0

    fwcset = cfg["image"]["target"]

    # Get media info for all devices
    media_list = _extract_media_info(cfg)

    # Build a partition lookup table across all devices
    # partition_table[part_name] = {"size": ..., "offset": ..., "device_name": ...,
    #                               "media_type": ..., "total_size": ...}
    partition_table = {}

    for media in media_list:
        device_name = media["name"]
        media_type = media["type"]

        if media_type not in ["spi-nand", "spi-nor", "mmc"]:
            print("Not supported media type: {}".format(media_type))
            return -1

        if device_name not in cfg:
            print("Device {} not found in config".format(device_name))
            return -1

        total_siz = size_str_to_int(cfg[device_name]["size"])
        partitions = cfg[device_name]["partitions"]
        if len(partitions) == 0:
            continue

        part_offs = 0
        for part in partitions:
            if "size" not in partitions[part]:
                print("No size value for partition: {}".format(part))
                return -1

            # get part size
            part_size = size_str_to_int(partitions[part]["size"])
            if partitions[part]["size"] == "-":
                part_size = total_siz - part_offs
            if "offset" in partitions[part]:
                part_offs = size_str_to_int(partitions[part]["offset"])

            # Handle UBI volumes
            if "ubi" in partitions[part]:
                volumes = partitions[part]["ubi"]
                if len(volumes) == 0:
                    print("Volume of {} is empty".format(part))
                    return -1
                vol_offs = part_offs
                for vol in volumes:
                    if "size" not in volumes[vol]:
                        print("No size value for ubi volume: {}".format(vol))
                        return -1
                    vol_size = size_str_to_int(volumes[vol]["size"])
                    if volumes[vol]["size"] == "-":
                        vol_size = part_size
                    if "offset" in volumes[vol]:
                        vol_offs = size_str_to_int(volumes[vol]["offset"])
                    vol_name = part + ":" + vol
                    partition_table[vol_name] = {
                        "size": vol_size,
                        "offset": vol_offs,
                        "device_name": device_name,
                        "media_type": media_type,
                        "total_size": total_siz
                    }
                    vol_offs += vol_size
                part_offs = vol_offs
            else:
                partition_table[part] = {
                    "size": part_size,
                    "offset": part_offs,
                    "device_name": device_name,
                    "media_type": media_type,
                    "total_size": total_siz
                }
                part_offs += part_size

    # Match targets with partitions
    for fwc in fwcset:
        if fwcset[fwc]["part"][0] in partition_table:
            part_info = partition_table[fwcset[fwc]["part"][0]]
            fwcset[fwc]["part_size"] = part_info["size"]
            fwcset[fwc]["part_offset"] = part_info["offset"]
            fwcset[fwc]["device_name"] = part_info["device_name"]
            fwcset[fwc]["media_type"] = part_info["media_type"]
            fwcset[fwc]["total_size"] = part_info["total_size"]
        else:
            print("Partition {} not found in any device".format(fwcset[fwc]["part"][0]))
            return -1

    return 0


def round_up(x, y):
    return int((x + y - 1) / y) * y


def aic_create_parts_for_env(cfg):
    mtd_list = []
    ubi_list = []
    gpt_list = []

    part_str = ""

    # Get media info for all devices
    media_list = _extract_media_info(cfg)

    for media in media_list:
        device_name = media["name"]
        media_type = media["type"]
        ctrl_id = media["controller"]

        if media_type == "spi-nand" or media_type == "spi-nor":
            partitions = cfg[device_name]["partitions"]
            mtd = "spi{}.0:".format(ctrl_id)
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)
            for part in partitions:
                itemstr = ""
                if "size" not in partitions[part]:
                    print("No size value for partition: {}".format(part))
                itemstr += partitions[part]["size"]
                if "offset" in partitions[part]:
                    itemstr += "@{}".format(partitions[part]["offset"])
                itemstr += "({})".format(part)
                mtd += itemstr + ","
                if "ubi" in partitions[part]:
                    volumes = partitions[part]["ubi"]
                    if len(volumes) == 0:
                        print("Volume of {} is empty".format(part))
                        sys.exit(1)
                    ubi = "{}:".format(part)
                    for vol in volumes:
                        itemstr = ""
                        if "size" not in volumes[vol]:
                            print("No size value for ubi volume: {}".format(vol))
                        itemstr += volumes[vol]["size"]
                        if "offset" in volumes[vol]:
                            itemstr += "@{}".format(volumes[vol]["offset"])
                        itemstr += "({})".format(vol)
                        ubi += itemstr + ","
                    ubi = ubi[0:-1]
                    ubi_list.append(ubi)
            mtd = mtd[0:-1]
            mtd_list.append(mtd)
        elif media_type == "mmc":
            partitions = cfg[device_name]["partitions"]
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)
            gpt = ""
            for part in partitions:
                itemstr = ""
                if "size" not in partitions[part]:
                    print("No size value for partition: {}".format(part))
                itemstr += partitions[part]["size"]
                if "offset" in partitions[part]:
                    itemstr += "@{}".format(partitions[part]["offset"])
                itemstr += "({})".format(part)
                gpt += itemstr + ","
            gpt = gpt[0:-1]
            gpt_list.append(gpt)
        else:
            print("Not supported media type: {}".format(media_type))
            sys.exit(1)

    # Build final partition string
    if mtd_list:
        part_str = "MTD={}".format(";".join(mtd_list))
        if ubi_list:
            part_str += "\nUBI={}".format(";".join(ubi_list))
        part_str += "\n"

    if gpt_list:
        gpt_str = ";".join(gpt_list)
        part_str += "GPT={}\nparts_mmc={}\n".format(gpt_str, gpt_str)

    return part_str


def uboot_env_create_image(srcfile, outfile, size, part_str, redund, script_dir):
    tmpfile = srcfile + ".part.tmp"
    fs = open(srcfile, "r+")
    envstr = fs.readlines()
    fs.close()
    fp = open(tmpfile, "w+")
    fp.write(part_str)
    fp.writelines(envstr)
    fp.close()

    mkenvcmd = os.path.join(script_dir, "mkenvimage")
    if os.path.exists(mkenvcmd) is False:
        mkenvcmd = "mkenvimage"
    if sys.platform == "win32":
        mkenvcmd += ".exe"
    if "enable" in redund:
        cmd = [mkenvcmd, "-r", "-s", str(size), "-o", outfile, tmpfile]
    else:
        cmd = [mkenvcmd, "-s", str(size), "-o", outfile, tmpfile]
    ret = subprocess.run(cmd, subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)


def get_pre_process_cfg(cfg):
    if "temporary" in cfg:
        return cfg["temporary"]
    elif "pre-process" in cfg:
        return cfg["pre-process"]
    return None


def firmware_component_preproc_itb(cfg, datadir, keydir, bindir):
    # Need to generate FIT image
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["itb"].keys()
    for itbname in imgnames:
        itsname = preproc_cfg["itb"][itbname]["its"]
        outfile = datadir + itbname
        dtbfile = None
        keypath = None

        if VERBOSE:
            print("\tCreating {} ...".format(outfile))
        srcfile = get_file_path(itsname, datadir)
        if srcfile is None:
            print("File {} is not exist".format(itsname))
            sys.exit(1)
        if "dtb" in preproc_cfg["itb"][itbname].keys():
            dtbname = preproc_cfg["itb"][itbname]["dtb"]
            dtbfile = get_file_path(dtbname, datadir)
            if dtbfile is None:
                print("File {} is not exist".format(dtbname))
                sys.exit(1)
        if "keydir" in preproc_cfg["itb"][itbname].keys():
            keydir = preproc_cfg["itb"][itbname]["keydir"]
            keypath = get_file_path(keydir, datadir)
            if keypath is None:
                print("File {} is not exist".format(keydir))

        itb_create_image(srcfile, outfile, keypath, dtbfile, bindir)

        # Generate a spl image with spl dtb file
        if "bin" in preproc_cfg["itb"][itbname].keys():
            srcbin = preproc_cfg["itb"][itbname]["bin"]["src"]
            dstbin = preproc_cfg["itb"][itbname]["bin"]["dst"]
            srcfile = get_file_path(srcbin, datadir)
            if srcfile is None:
                print("File {} is not exist".format(srcbin))
                sys.exit(1)
            dstfile = get_file_path(dstbin, datadir)
            if dstfile is None:
                print("File {} is not exist".format(dstbin))
                sys.exit(1)
            cmd = ["cat {} {} > {}".format(srcfile, dtbfile, dstfile)]
            ret = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
            if ret.returncode != 0:
                sys.exit(1)


def firmware_component_preproc_elf2itb(cfg: dict, datadir: str,
                                       keydir: str, bindir: str) -> None:
    """Process elf2itb config: ELF -> ITS (via elf2image) -> ITB (via mkimage)."""
    preproc_cfg = get_pre_process_cfg(cfg)
    elf2itb_cfg = preproc_cfg["elf2itb"]

    # Get firmware version from image config
    version = cfg.get("image", {}).get("info", {}).get("version", "1.0.0")

    for itbname, entry in elf2itb_cfg.items():
        elf_file = entry["elf"]
        elf_path = get_file_path(elf_file, datadir)
        if elf_path is None:
            print("File {} is not exist".format(elf_file))
            sys.exit(1)

        # Derive ITS filename from ITB name: d12x_os.itb -> d12x_os.its
        its_name = itbname.replace(".itb", ".its")
        its_path = datadir + its_name
        itb_path = datadir + itbname

        if VERBOSE:
            print("\tCreating {} from {} ...".format(itbname, elf_file))

        # Convert hash config (list of algorithm names)
        hash_algos = entry.get("hash", None)

        # Convert signature config for ITS
        sig_cfg = None
        sig_keydir = None
        if "signature" in entry:
            sig = entry["signature"]
            key_path = sig.get("key", "")
            key_hint = os.path.splitext(os.path.basename(key_path))[0]
            sig_cfg = {"algo": sig["algo"], "key-name-hint": key_hint}
            # Resolve keydir for mkimage signing
            resolved = get_file_path(key_path, keydir)
            sig_keydir = os.path.dirname(resolved) if resolved else keydir

        # Convert encryption config for ITS
        # mkimage cipher node expects:
        #   algo: "aes128" / "aes192" / "aes256" (not "aes-128-cbc")
        #   key-name-hint: basename without ext, mkimage reads {keydir}/{hint}.bin
        #   iv-name-hint:  basename without ext, mkimage reads {keydir}/{hint}.bin
        _ITS_ALGO_MAP = {
            "aes-128-cbc": "aes128",
            "aes-192-cbc": "aes192",
            "aes-256-cbc": "aes256",
            "chacha20": "chacha20",
        }
        enc_cfg = None
        enc_keydir = None
        if "encryption" in entry:
            enc = entry["encryption"]
            its_algo = _ITS_ALGO_MAP.get(enc["algo"], enc["algo"])
            enc_cfg = {"algo": its_algo}
            if "key" in enc:
                key_path = enc["key"]
                enc_cfg["key-name-hint"] = os.path.splitext(
                    os.path.basename(key_path))[0]
                resolved = get_file_path(key_path, keydir)
                enc_keydir = os.path.dirname(resolved) if resolved else keydir
            if "iv" in enc:
                iv_path = enc["iv"]
                enc_cfg["iv-name-hint"] = os.path.splitext(
                    os.path.basename(iv_path))[0]

        # Step 1: Generate .its + seg*.bin via elf2image
        elf2its(
            elf_file=elf_path,
            version=version,
            its_file=its_path,
            hash_algos=hash_algos,
            signature=sig_cfg,
            encryption=enc_cfg,
        )

        # Step 2: Generate .itb from .its via mkimage
        # Use enc_keydir if sig_keydir is not set (cipher needs -k too)
        mkimage_keydir = sig_keydir if sig_keydir else enc_keydir
        dtbfile = None
        if "dtb" in entry:
            dtbfile = get_file_path(entry["dtb"], datadir)
        itb_create_image(its_path, itb_path, mkimage_keydir, dtbfile, bindir)


def firmware_component_preproc_uboot_env(cfg, datadir, keydir, bindir):
    # Need to generate uboot env bin
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["uboot_env"].keys()
    part_str = aic_create_parts_for_env(cfg)
    envredund = "disable"
    for binname in imgnames:
        envfile = preproc_cfg["uboot_env"][binname]["file"]
        envsize = preproc_cfg["uboot_env"][binname]["size"]
        if "redundant" in preproc_cfg["uboot_env"][binname]:
            envredund = preproc_cfg["uboot_env"][binname]["redundant"]
        outfile = datadir + binname
        if VERBOSE:
            print("\tCreating {} ...".format(outfile))
        srcfile = get_file_path(envfile, datadir)
        if srcfile is None:
            print("File {} is not exist".format(envfile))
            sys.exit(1)
        uboot_env_create_image(srcfile, outfile, envsize, part_str,
                               envredund, bindir)


def firmware_component_preproc_aicboot(cfg, datadir, keydir, bindir):
    # Legacy code, should not use after Luban-Lite 1.0.6 and Luban SDK 1.2.5
    # Need to generate aicboot image
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["aicboot"].keys()
    for name in imgnames:
        imgcfg = preproc_cfg["aicboot"][name]
        imgcfg["keydir"] = keydir
        imgcfg["datadir"] = datadir
        imgcfg["_outname"] = name
        outname = datadir + name
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        imgbytes = aic_boot_create_image(imgcfg, keydir, datadir)

        if check_loader_run_in_dram(imgcfg):
            extimgbytes = aic_boot_create_ext_image(imgcfg, keydir, datadir)
            padlen = round_up(len(imgbytes), META_ALIGNED_SIZE) - len(imgbytes)
            if padlen > 0:
                imgbytes += bytearray(padlen)
            imgbytes += extimgbytes
            # For Debug
            # with open(outname + ".ext", "wb") as f:
            #     f.write(extimgbytes)

        with open(outname, "wb") as f:
            f.write(imgbytes)


def firmware_component_preproc_aicimage(cfg, datadir, keydir, bindir):
    # Need to generate aicboot image
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["aicimage"].keys()
    for name in imgnames:
        imgcfg = preproc_cfg["aicimage"][name]
        imgcfg["keydir"] = keydir
        imgcfg["datadir"] = datadir
        imgcfg["_outname"] = name
        outname = datadir + name
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        imgbytes = aic_boot_create_image_v2(imgcfg, keydir, datadir)
        with open(outname, "wb") as f:
            f.write(imgbytes)


def firmware_component_preproc_spienc(cfg, datadir, keydir, bindir):
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["spienc"].keys()
    for name in imgnames:
        imgcfg = preproc_cfg["spienc"][name]
        imgcfg["keydir"] = keydir
        imgcfg["datadir"] = datadir
        outname = datadir + name
        imgcfg["input"] = datadir + imgcfg["file"]
        imgcfg["output"] = outname
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        spienc_create_image(imgcfg, bindir)


def firmware_component_preproc_spienctool(cfg, datadir, keydir, bindir):
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["spienctool"].keys()
    for name in imgnames:
        imgcfg = preproc_cfg["spienctool"][name]
        imgcfg["keydir"] = keydir
        imgcfg["datadir"] = datadir
        outname = datadir + name
        imgcfg["input"] = datadir + imgcfg["file"]
        imgcfg["output"] = outname
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        spienctool_key_encrypt(imgcfg, bindir)


def firmware_component_preproc_data_crypt(cfg, datadir, keydir, bindir):
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["data_crypt"].keys()
    for name in imgnames:
        imgcfg = preproc_cfg["data_crypt"][name]
        imgcfg["keydir"] = keydir
        imgcfg["datadir"] = datadir
        outname = datadir + name
        imgcfg["input"] = datadir + imgcfg["file"]
        imgcfg["output"] = outname
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        data_crypt_create_image(imgcfg, bindir)


def firmware_component_preproc_aicprivate(cfg: dict, datadir: str,
                                          keydir: str, bindir: str) -> None:
    """Process aicprivate config: generate private resource binary from JSON configs."""
    preproc_cfg = get_pre_process_cfg(cfg)
    aicprivate_cfg = preproc_cfg["aicprivate"]

    for name, entry in aicprivate_cfg.items():
        cfg_files = entry.get("cfg", [])
        outpath = datadir + name

        if VERBOSE:
            print("\tCreating {} ...".format(outpath))

        # Resolve each config file path relative to datadir
        resolved: list = []
        for cf in cfg_files:
            p = get_file_path(cf, datadir)
            if p is None:
                print("File {} is not exist".format(cf))
                sys.exit(1)
            resolved.append(p)

        # Parse, merge, build, and write
        merged: dict = {}
        for p in resolved:
            merged.update(_priv_parse_config(p))
        data = _priv_build(merged, os.path.abspath(datadir))
        with open(outpath, "wb") as f:
            f.write(data)


def firmware_component_preproc_concatenate(cfg, datadir, keydir, bindir):
    preproc_cfg = get_pre_process_cfg(cfg)
    imgnames = preproc_cfg["concatenate"].keys()
    for name in imgnames:
        outname = datadir + name
        if VERBOSE:
            print("\tCreating {} ...".format(outname))
        flist = preproc_cfg["concatenate"][name]
        if isinstance(flist, list):
            concatenate_create_image(outname, flist, datadir)
        else:
            print("\tWarning: {} in \'concatenate' is not list".format(name))
            continue


def firmware_component_preproc(cfg, datadir, keydir, bindir):
    """ Perform firmware component pre-process
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
        keydir: key material directory for image data
    """
    preproc_cfg = get_pre_process_cfg(cfg)
    if preproc_cfg is None:
        return None
    preprocs = preproc_cfg.keys()
    for proc in preprocs:
        if proc == "aicprivate":
            firmware_component_preproc_aicprivate(cfg, datadir, keydir, bindir)
        if proc == "itb":
            firmware_component_preproc_itb(cfg, datadir, keydir, bindir)
        if proc == "elf2itb":
            firmware_component_preproc_elf2itb(cfg, datadir, keydir, bindir)
        if proc == "uboot_env":
            firmware_component_preproc_uboot_env(cfg, datadir, keydir, bindir)
        if proc == "aicboot":
            # Legacy code
            firmware_component_preproc_aicboot(cfg, datadir, keydir, bindir)
        if proc == "aicimage":
            firmware_component_preproc_aicimage(cfg, datadir, keydir, bindir)
        if proc == "concatenate":
            firmware_component_preproc_concatenate(cfg, datadir, keydir, bindir)
        if proc == "spienc":
            firmware_component_preproc_spienc(cfg, datadir, keydir, bindir)
        if proc == "spienctool":
            firmware_component_preproc_spienctool(cfg, datadir, keydir, bindir)
        if proc == "data_crypt":
            firmware_component_preproc_data_crypt(cfg, datadir, keydir, bindir)


def generate_bootcfg(bcfgfile, cfg):
    comments = ["# Boot configuration file\n",
                "# Used in SD Card FAT32 boot and USB Disk upgrade.\n",
                "# Format:\n",
                "# protection=part1 name,part2 name,part3 name\n",
                "#   Protects partitions from being overwritten when they are upgraded.\n"
                "# boot0=size@offset\n",
                "#   boot0 size and location offset in 'image' file, boot rom read it.\n"
                "# boot0=example.bin\n",
                "#   boot0 image is file example.bin, boot rom read it.\n"
                "# boot1=size@offset\n",
                "#   boot1 size and location offset in 'image' file, boot0 read it.\n"
                "# boot1=example.bin\n",
                "#   boot1 image is file example.bin, boot0 read it.\n"
                "# image=example.img\n",
                "#   Packed image file is example.img, boot1 use it.\n",
                "# \n",
                "# For Direct Mode.\n",
                "# boot0=bootloader.aic\n",
                "# writetype=spi-nor\n",
                "#           Value can be: spi-nor spi-nand mmc\n",
                "# writeintf=0\n",
                "#           Default is 0 if this key is not provided\n",
                "# writeboot=bootloader.aic\n",
                "#           It is required for spi-nand to update bootloader.\n",
                "# write0=data0.bin,0x1000\n",
                "#           writeX=file,offset,attribute\n",
                "#           writeX=file,offset\n",
                "#           writeX=file\n",
                "#           X can be 0 ~ 31\n",
                "#           if offset is absent, default value is 0\n",
                "#           e.g.: write3=data.fatfs,0x100000,nftl\n",
                "\n\n",
                ]
    bytes_comments = [comment.encode() for comment in comments]
    bcfgfile.writelines(bytes_comments)

    fwcset = cfg["image"]["updater"]
    fwckeys = cfg["image"]["updater"].keys()
    if "spl" in fwckeys:
        fwcname = "spl"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr.encode())
        linestr = "boot0={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                         hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr.encode())

    if "uboot" in fwckeys:
        fwcname = "uboot"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr.encode())
        linestr = "boot1={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                         hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr.encode())

    if "env" in fwckeys:
        fwcname = "env"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr.encode())
        linestr = "env={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                       hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr.encode())

    imgfn = img_gen_fw_file_name(cfg)
    linestr = "image={}\n".format(imgfn)
    bcfgfile.write(linestr.encode())
    bcfgfile.flush()


def get_spinand_image_list(cfg, datadir):
    imglist = []
    orglist = cfg["image"]["info"]["media"]["array_organization"]
    for item in orglist:
        paramstr = "_page_{}_block_{}".format(item["page"], item["block"])
        paramstr = paramstr.lower()
        status_ok = True
        for fwcname in cfg["image"]["target"]:
            if "ubi" not in cfg["image"]["target"][fwcname]["attr"]:
                # Not UBI partition
                continue
            # UBI component
            filepath = cfg["image"]["target"][fwcname]["file"]
            if filepath.find("*") <= 0:
                # No need to check
                continue
            filepath = filepath.replace("*", paramstr)
            filepath = get_file_path(filepath, datadir)
            if filepath is None and "optional" not in cfg["image"]["target"][fwcname]["attr"]:
                # FWC file not exist
                status_ok = False
                print("{} is not found".format(cfg["image"]["target"][fwcname]["file"]))
                break
            backup = cfg["image"]["target"][fwcname]["file"]
            # Backup the original file path string, because it will be modified
            # when generating image
            cfg["image"]["target"][fwcname]["file.backup"] = backup
        if status_ok:
            imglist.append(paramstr)
    backup = cfg["image"]["info"]["product"]
    cfg["image"]["info"]["product.backup"] = backup
    backup = cfg["image"]["bootcfg"]
    cfg["image"]["bootcfg.backup"] = backup
    backup = cfg["image"]["part_table"]
    cfg["image"]["part_table.backup"] = backup
    return imglist, orglist


def fixup_spinand_ubi_fwc_name(cfg, paramstr, orgitem):
    for fwcname in cfg["image"]["target"]:
        if "ubi" not in cfg["image"]["target"][fwcname]["attr"]:
            # Not UBI partition
            continue
        # UBI component
        filepath = cfg["image"]["target"][fwcname]["file.backup"]
        if filepath.find("*") <= 0:
            # No need to fixup
            continue
        cfg["image"]["target"][fwcname]["file"] = filepath.replace("*", paramstr)
    # fixup others
    backup = cfg["image"]["info"]["product.backup"]
    cfg["image"]["info"]["product"] = backup + paramstr
    backup = cfg["image"]["bootcfg.backup"]
    cfg["image"]["bootcfg"] = "{}({})".format(backup, paramstr[1:])
    backup = cfg["image"]["part_table.backup"]
    cfg["image"]["part_table"] = "{}({})".format(backup, paramstr[1:])
    cfg["image"]["info"]["media"]["array_organization"] = [orgitem]


def build_pinmux_check(cfg, image_path):
    # FPGA-type boards may not have an aicboot key, in which case the pinmux
    # conflict checking exited directly.
    preproc_cfg = get_pre_process_cfg(cfg)
    if preproc_cfg is None:
        return 0

    cwd = os.getcwd()

    target_path = image_path.replace('images', 'target')
    preproc_path = os.path.join(cwd, 'output', image_path, '.pinmux.i')
    if not os.path.exists(preproc_path):
        return 0

    if (cfg["image"]["info"].get("product.backup")):
        prduct = cfg["image"]["info"]["product.backup"].replace("_", "-")
        rel_pinmux_path = os.path.join('target',
                                       cfg["image"]["info"]["platform"],
                                       prduct, 'pinmux.c')
    else:
        prduct = cfg["image"]["info"]["product"].replace("_", "-")
        rel_pinmux_path = os.path.join('target',
                                       cfg["image"]["info"]["platform"],
                                       prduct, 'pinmux.c')
    pinmux_path = os.path.join(cwd, rel_pinmux_path)
    root_path = target_path.replace(os.path.join(cwd, 'output'), '')
    if platform.system() == 'Linux':
        defconfig_name = root_path.replace('target', '').replace(os.path.sep, '') + '_defconfig'
        defconfig_path = os.path.join(cwd, 'target', 'configs', defconfig_name)
    elif platform.system() == 'Windows':
        defconfig_name = re.sub(r'/+', '', root_path).replace('target', '') + '_defconfig'
        defconfig_path = os.path.join(cwd, 'target', 'configs', defconfig_name)

    list_preproc_pins = []
    list_conflict_pins = []
    dict_pinmux = {}

    # Get all configured pins and multiplexed functions in the pre-processed file pinmux.i
    with open(preproc_path, 'r') as file:
        pin_pattern = r'\{(\d+),\s*([^,]+),\s*(\d+),\s*("[^"]+"|[^,]+)(,\s*\(\d+\))?\}'
        for f in file:
            match = re.search(pin_pattern, f)
            if match:
                list_preproc_pins.append([match.groups()[0], match.groups()[3]])
    file.close()

    # Get the dictionary key as pin_name and the value as an array containing
    # all the currently multiplexed functions.
    # Tips: When the length of the value in the dictionary is greater than 1,
    # it indicates that the pin is multiplexed with multiple functions.
    for row in list_preproc_pins:
        if row[1] not in dict_pinmux:
            dict_pinmux[row[1]] = [row[0]]
        else:
            dict_pinmux[row[1]].append(row[0])
    for pin_name, pin_func in dict_pinmux.items():
        if len(pin_func) > 1:
            list_conflict_pins.append(pin_name)

    if len(list_conflict_pins) == 0:
        return 0

    # Print macro definitions based on pins of conflict
    pr_warn("Current pinmux conflicts! The conflicting pin:")
    lines_num = 0
    max_pin_name = max(len(s) for s in list_conflict_pins)
    pin_name_total_len = max_pin_name + 2
    enabled_macro = {}

    with open(defconfig_path, 'r') as file:
        matched_num = 0
        for f in file:
            for i in range(len(list_conflict_pins)):
                match = re.search(list_conflict_pins[i], f)
                if not match:
                    continue
                matched_num += 1
                if matched_num == 1:
                    print("\n{:<{}}".format('PIN', pin_name_total_len), end='')
                    print('MACROS (' + defconfig_name + ')')
                print("{:<{}}".format(list_conflict_pins[i].replace("\"",
                      "") + ': ', pin_name_total_len), end='')
                print(f.split('=')[0])
                key_pin_name = f.split('=')[1].split('\n')[0]
                val_macro = f.split('=')[0].replace('CONFIG_', '')
                if key_pin_name in enabled_macro:
                    enabled_macro[key_pin_name].append(val_macro)
                else:
                    enabled_macro[key_pin_name] = [val_macro]
    file.close()
    print("\n{:<{}}".format('PIN', pin_name_total_len), end='')
    print('LINES (' + rel_pinmux_path + ')')

    # Print the line number of conflicting pins in pinmux.c file
    with open(pinmux_path, 'r') as file:
        lines = file.readlines()
        total_lines = len(str(len(lines))) + 2
        file.seek(0)
        for i in range(len(list_conflict_pins)):
            print("{:<{}}".format(list_conflict_pins[i].replace("\"",
                  "") + ': ', pin_name_total_len), end='')
            pin_func = dict_pinmux.get(list_conflict_pins[i])
            matched_num = 0

            for f in file:
                lines_num += 1
                match = re.search(list_conflict_pins[i], f)
                if not match:
                    continue

                fun = f.split('{')[1].split(',')[0]
                if fun in pin_func:
                    matched_num += 1
                    if matched_num > 1:
                        print(' ' * pin_name_total_len, end='')
                    line_str = str(lines_num) + ': '
                    print("{:<{}}".format(line_str, total_lines), end='')
                    print(f.replace(' ', ''), end='')
            file.seek(0)
            lines_num = 0

            # Search backwards from the macro to the line where the pin
            # function configuration is
            if list_conflict_pins[i] in enabled_macro:
                for pin_name_index in enabled_macro[list_conflict_pins[i]]:
                    lines_num_macro = 0
                    for f in file:
                        lines_num_macro += 1
                        match = re.search(pin_name_index + '}', f)
                        line_str = str(lines_num_macro) + ': '
                        if not match:
                            continue
                        print(' ' * pin_name_total_len, end='')
                        print("{:<{}}".format(line_str, total_lines), end='')
                        print(f.replace(' ', ''), end='')
                    file.seek(0)
                    lines_num_macro = 0
    file.close()


def build_firmware_image(cfg, datadir, outdir):
    """ Build firmware image
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    # Step0: Get all part size
    ret = img_get_part_size(cfg, datadir)
    if ret != 0:
        return ret

    # Step1: Get all FWC file's size
    ret = img_get_fwc_file_size(cfg, datadir)
    if ret != 0:
        return ret

    # Step2: Calculate Meta Area's size, one FWC use DATA_ALIGNED_SIZE bytes
    meta_area_size = 0
    for s in ["updater", "target"]:
        fwcset = cfg["image"][s]
        for fwc in fwcset:
            if fwcset[fwc]["filesize"] > 0:
                meta_area_size += META_ALIGNED_SIZE
    # Image header is also one FWC, it need one FWC Meta
    meta_area_size += META_ALIGNED_SIZE

    # Step3: Calculate the size of FWC File Data Area
    file_area_size = 0
    for s in ["updater", "target"]:
        if s in cfg["image"] is False:
            return -1
        for fwc in cfg["image"][s]:
            if "filesize" in cfg["image"][s][fwc] is False:
                return -1
            filesize = cfg["image"][s][fwc]["filesize"]
            if filesize > 0:
                filesize = round_up(filesize, DATA_ALIGNED_SIZE)
                file_area_size += filesize

    # Step4: Create empty image file
    img_fn = datadir + img_gen_fw_file_name(cfg)
    img_total_size = DATA_ALIGNED_SIZE # Header
    img_total_size += meta_area_size
    img_total_size += file_area_size
    with open(img_fn, 'wb') as imgfile:
        imgfile.truncate(img_total_size)
        # Step5: Write header
        ret = img_write_fw_header(imgfile, cfg, meta_area_size, file_area_size)
        if ret != 0:
            return ret
        # Step6: Write FW Component meta to image
        meta_start = DATA_ALIGNED_SIZE
        file_start = meta_start + meta_area_size
        ret = img_write_fwc_meta_to_imgfile(imgfile, cfg, meta_start,
                                            file_start, datadir)
        if ret != 0:
            return ret
        # Step7: Write FW Component file data to image
        ret = img_write_fwc_file_to_imgfile(imgfile, cfg, file_start, datadir)
        if ret != 0:
            return ret
        imgfile.flush()

    abspath = "{}".format(img_fn)
    (img_path, img_name) = os.path.split(abspath)
    if VERBOSE:
        print("\tImage file is generated: {}/{}\n\n".format(img_path, img_name))

    if BURNER:
        os.makedirs(outdir + "burner/", exist_ok=True)
        # Get media info
        media_list = _extract_media_info(cfg)
        img_bin_name = img_gen_fw_file_name(cfg).replace(".img", ".bin")
        part_table_name = cfg["image"]["part_table"]

        for idx, media in enumerate(media_list):
            # One bin file for each flash device
            if len(media_list) > 1 and idx > 0:
                suffix = "_{}".format(media["name"])
            else:
                suffix = ""
            img_bin_fn = outdir + "burner/" + img_bin_name.replace(".bin",
                                                                   suffix + ".bin")
            part_table_fn = outdir + "burner/" + part_table_name.replace(".bin",
                                                                         suffix + ".bin")

            with open(img_bin_fn, 'wb') as binfile:
                # Only spi-nand need gen page table
                if media["type"] == "spi-nand":
                    ret = img_gen_page_table(binfile, cfg, datadir)
                    if ret != 0:
                        return ret

                ret = img_write_fwc_file_to_binfile(binfile, cfg, datadir,
                                                    media["name"], part_table_fn)
                if ret != 0:
                    return ret

            abspath = "{}".format(img_bin_fn)
            (img_path, img_name) = os.path.split(abspath)
            if VERBOSE:
                print("\tImage bin file is generated: {}/{}\n".format(img_path, img_name))

    bootcfg_fn = datadir + cfg["image"]["bootcfg"]
    with open(bootcfg_fn, 'wb') as bcfgfile:
        generate_bootcfg(bcfgfile, cfg)
        bcfgfile.flush()
    # Always set page_2k_block_128k nand image as default
    if "page_2k_block_128k" in bootcfg_fn:
        default_bootcfg_fn = bootcfg_fn.replace('(page_2k_block_128k)', '')
        with open(default_bootcfg_fn, 'wb') as bcfgfile:
            generate_bootcfg(bcfgfile, cfg)
            bcfgfile.flush()

    build_pinmux_check(cfg, img_path)
    return 0


def make_untar(src_file, out_dir):
    with tarfile.open(src_file) as t:
        t.extractall(out_dir)
        print("Extract {} file data to {}".format(src_file, out_dir))


def extract_img_meta_data(imgfile, datadir, meta_off):
    magic = str_from_nbytes(imgfile.read(8))
    name = str_from_nbytes(imgfile.read(64))
    partition = str_from_nbytes(imgfile.read(64))
    offset = int_from_uint32_bytes(imgfile.read(4))
    size = int_from_uint32_bytes(imgfile.read(4))
    crc = int_from_uint32_bytes(imgfile.read(4))
    ram = int_from_uint32_bytes(imgfile.read(4))
    attr = str_from_nbytes(imgfile.read(64))
    filename = str_from_nbytes(imgfile.read(64)).strip(b'\x00'.decode())

    imgfile.seek(offset)
    pathfile = datadir + '/' + filename
    with open(pathfile, 'wb') as f:
        f.write(imgfile.read(size))

    if Path(pathfile).suffix == ".tar":
        make_untar(pathfile, datadir)


def extract_img_data(img):
    datadir = os.path.join(os.path.dirname(img), Path(img).stem)
    os.makedirs(datadir, exist_ok=True)
    with open(img, 'rb+') as imgfile:
        magic = str_from_nbytes(imgfile.read(8))
        platform = str_from_nbytes(imgfile.read(64))
        product = str_from_nbytes(imgfile.read(64))
        version = str_from_nbytes(imgfile.read(64))
        media_type = str_from_nbytes(imgfile.read(64))
        media_dev_id = int_from_uint32_bytes(imgfile.read(4))
        nand_array_org = str_from_nbytes(imgfile.read(64))
        meta_offset = int_from_uint32_bytes(imgfile.read(4))
        meta_size = int_from_uint32_bytes(imgfile.read(4))
        file_offset = int_from_uint32_bytes(imgfile.read(4))
        file_size = int_from_uint32_bytes(imgfile.read(4))
        ex_flag = int_from_uint32_bytes(imgfile.read(4))
        ex_offset = int_from_uint32_bytes(imgfile.read(4))
        ex_size = int_from_uint32_bytes(imgfile.read(4))

        count = (int)(meta_size / META_ALIGNED_SIZE)
        for i in range(0, count):
            imgfile.seek(meta_offset)
            extract_img_meta_data(imgfile, datadir, meta_offset)
            meta_offset += META_ALIGNED_SIZE

    return datadir


if __name__ == "__main__":
    default_bin_root = os.path.dirname(sys.argv[0])
    if sys.platform.startswith("win"):
        default_bin_root = os.path.dirname(sys.argv[0]) + "/"
    parser = argparse.ArgumentParser()
    group = parser.add_mutually_exclusive_group()
    group.add_argument("-d", "--datadir", type=str,
                       help="input image data directory")
    group.add_argument("-i", "--imgfile", type=str,
                       help="input unsigned image file")
    parser.add_argument("-o", "--outdir", type=str,
                        help="output image file dir")
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-k", "--keydir", type=str,
                        help="key material directory")
    parser.add_argument("-e", "--extract", action='store_true',
                        help="extract extension file")
    parser.add_argument("-s", "--sign", action='store_true',
                        help="sign image file")
    parser.add_argument("-b", "--burner", action='store_true',
                        help="generate burner format image")
    parser.add_argument("-p", "--preprocess", action='store_true',
                        help="run preprocess only")
    parser.add_argument("-v", "--verbose", action='store_true',
                        help="show detail information")
    args = parser.parse_args()
    # If user not specified data directory, use current directory as default
    if args.datadir is None:
        args.datadir = './'
    if args.outdir is None:
        args.outdir = args.datadir
    if args.datadir.endswith('/') is False and args.datadir.endswith('\\') is False:
        args.datadir = args.datadir + '/'
    if args.outdir.endswith('/') is False and args.outdir.endswith('\\') is False:
        args.outdir = args.outdir + '/'
    if args.imgfile:
        args.datadir = extract_img_data(args.imgfile) + '/'
    if args.config is None:
        args.config = args.datadir + "image_cfg.json"
    if args.keydir is None:
        args.keydir = args.datadir
    if args.keydir.endswith('/') is False and args.keydir.endswith('\\') is False:
        args.keydir = args.keydir + '/'
    if args.burner:
        BURNER = True
    if args.verbose:
        VERBOSE = True
    if args.extract:
        sys.exit(1)
    if args.config is None:
        print('Error, option --config is required.')
        sys.exit(1)

    cfg = parse_image_cfg(args.config)
    # Pre-process here, e.g: signature, encryption, ...
    if get_pre_process_cfg(cfg) is not None:
        firmware_component_preproc(cfg, args.datadir, args.keydir, default_bin_root)
    if args.preprocess:
        sys.exit(0)

    cfg["image"]["bootcfg"] = "bootcfg.txt"
    cfg["image"]["part_table"] = "image_part_table.bin"
    # Finally build the firmware image
    imglist = []

    # Get media info
    media_list = _extract_media_info(cfg)
    media_type = media_list[0]["type"]

    if media_type == "spi-nand":
        imglist, orglist = get_spinand_image_list(cfg, args.datadir)
    if len(imglist) > 0:
        # SPI-NAND UBI case
        for imgitem, orgitem in zip(imglist, orglist):
            # fixup file path
            fixup_spinand_ubi_fwc_name(cfg, imgitem, orgitem)
            ret = build_firmware_image(cfg, args.datadir, args.outdir)
            if ret != 0:
                sys.exit(1)
    else:
        # Just create image, no need to fixup anything
        ret = build_firmware_image(cfg, args.datadir, args.outdir)
        if ret != 0:
            sys.exit(1)
