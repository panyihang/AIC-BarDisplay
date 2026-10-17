# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._sign — RSA signing and public-key injection for FIT images.

Corresponds to C functions in ``u-boot/tools/image-host.c``:
  - ``fit_image_process_sig()``  — image-level signing
  - ``fit_config_process_sig()`` — configuration-level signing
  - ``rsa_add_verify_data()``    — public-key injection into DTB
"""
import hashlib
import os
import struct
import time
from typing import Dict, List, Optional, Tuple

import fdt


# ── Helpers ─────────────────────────────────────────────────────────

def _get_prop_str(node, name: str, default: str = '') -> str:
    prop = node.get_property(name)
    if prop is None:
        return default
    try:
        return prop[0]
    except (IndexError, TypeError):
        return default


def _get_image_data(node) -> bytes:
    """Extract raw bytes from an image node's ``data`` property."""
    prop = node.get_property("data")
    if prop is None:
        return b''
    if isinstance(prop, fdt.PropBytes):
        return bytes(prop.data)
    if isinstance(prop, fdt.PropWords):
        return b''.join(struct.pack(">I", v) for v in prop.data)
    return b''


def _parse_algo(algo_str: str) -> Tuple[str, str]:
    """Parse an algo string like ``'sha256,rsa2048'``.

    Returns ``(hash_name, crypto_name)``.
    """
    if ',' in algo_str:
        parts = algo_str.split(',', 1)
        return parts[0].strip(), parts[1].strip()
    # Default: sha256 hash, algo is the whole string
    return 'sha256', algo_str.strip()


def _load_rsa_private_key(keydir: str, key_name: str):
    """Load an RSA private key from ``{keydir}/{key_name}.key`` (PEM).

    Returns a ``Cryptodome.PublicKey.RSA.RsaKey`` object, or *None*
    if the file doesn't exist.
    """
    key_path = os.path.join(keydir, key_name + '.key')
    if not os.path.exists(key_path):
        return None
    from Cryptodome.PublicKey import RSA
    with open(key_path, 'rb') as fh:
        return RSA.import_key(fh.read())


def _rsa_sign_pkcs1v15(
    private_key,
    hash_name: str,
    data: bytes,
) -> bytes:
    """Sign *data* with RSA PKCS#1 v1.5.

    Returns the raw signature bytes.
    """
    from Cryptodome.Signature import pkcs1_15
    from Cryptodome.Hash import (
        SHA1, SHA256, SHA384, SHA512)

    hash_modules = {
        'sha1': SHA1,
        'sha256': SHA256,
        'sha384': SHA384,
        'sha512': SHA512,
    }
    h_mod = hash_modules.get(hash_name.lower())
    if h_mod is None:
        raise ValueError(f"Unsupported hash for RSA: {hash_name}")

    h = h_mod.new(data)
    signer = pkcs1_15.new(private_key)
    return signer.sign(h)


# ── Image Signing ───────────────────────────────────────────────────

def sign_images(fdt_obj: fdt.FDT, keydir: str) -> None:
    """Sign all image nodes that contain ``signature*`` sub-nodes.

    Corresponds to C ``fit_image_process_sig()``.
    """
    images_node = fdt_obj.get_node("images")
    if images_node is None:
        return

    for img_node in images_node.nodes:
        data = _get_image_data(img_node)
        if not data:
            continue

        for sub_node in list(img_node.nodes):
            if not sub_node.name.startswith("signature"):
                continue

            algo_str = _get_prop_str(sub_node, "algo")
            if not algo_str:
                continue
            hash_name, crypto_name = _parse_algo(algo_str)

            key_hint = _get_prop_str(sub_node, "key-name-hint")
            if not key_hint:
                continue

            priv_key = _load_rsa_private_key(keydir, key_hint)
            if priv_key is None:
                print(f"Warning: key '{key_hint}.key' not found "
                      f"in '{keydir}', skipping image signature")
                continue

            # Sign the image data
            try:
                sig = _rsa_sign_pkcs1v15(priv_key, hash_name, data)
            except Exception as exc:
                print(f"Warning: RSA sign failed for "
                      f"'{img_node.name}/{sub_node.name}': {exc}")
                continue

            # Write signature properties
            sub_node.set_property("value", sig)
            sub_node.set_property("signer-name", "mkimage")
            sub_node.set_property("signer-version", "1.0.0")
            ts = int(time.time())
            sub_node.set_property("timestamp", ts)


# ── Configuration Signing ───────────────────────────────────────────

def sign_configurations(fdt_obj: fdt.FDT, keydir: str) -> None:
    """Sign all configuration nodes that contain ``signature*``
    sub-nodes.

    Corresponds to C ``fit_config_process_sig()``.  This is the most
    complex part of FIT signing: it needs to extract specific DTB
    regions, hash them, and sign the hash.
    """
    try:
        confs_node = fdt_obj.get_node("configurations")
    except ValueError:
        return

    if confs_node is None:
        return

    for conf_node in confs_node.nodes:
        for sig_node in list(conf_node.nodes):
            if not sig_node.name.startswith("signature"):
                continue
            _sign_one_configuration(
                fdt_obj, conf_node, sig_node, keydir)


def _sign_one_configuration(
    fdt_obj: fdt.FDT,
    conf_node,
    sig_node,
    keydir: str,
) -> None:
    """Sign a single configuration signature node."""
    from itbimage._fdt_regions import find_regions

    algo_str = _get_prop_str(sig_node, "algo")
    if not algo_str:
        return
    hash_name, crypto_name = _parse_algo(algo_str)

    key_hint = _get_prop_str(sig_node, "key-name-hint")
    if not key_hint:
        return

    priv_key = _load_rsa_private_key(keydir, key_hint)
    if priv_key is None:
        print(f"Warning: key '{key_hint}.key' not found "
              f"in '{keydir}', skipping config signature")
        return

    # Determine which image types to sign
    sign_images_prop = _get_prop_str(sig_node, "sign-images")
    if sign_images_prop:
        sign_types = [s.strip() for s in sign_images_prop.split(',')]
    else:
        sign_types = ["firmware", "flat_dt", "script"]

    # Build the list of node paths to hash
    node_paths = _collect_config_hash_paths(
        fdt_obj, conf_node, sign_types)

    if not node_paths:
        print(f"Warning: no nodes to sign for config "
              f"'{conf_node.name}'")
        return

    # Exclude data-related properties from the hash
    exc_props = [
        "data", "data-size", "data-position", "data-offset",
    ]

    # Serialize the FDT (with signature value placeholder removed)
    # to get the DTB binary for region extraction
    dtb_data = fdt_obj.to_dtb(version=17)

    # Read the strings buffer for deterministic re-serialization
    from fdt.header import Header
    hdr = Header.parse(dtb_data)
    strings_off = hdr.off_dt_strings
    strings_sz = hdr.size_dt_strings or 0
    strings_buf = dtb_data[strings_off:strings_off + strings_sz]
    strings_str = strings_buf.decode('ascii', errors='replace')

    # Find regions
    regions, hashed_nodes = find_regions(
        dtb_data, node_paths, exc_props=exc_props,
        add_string_tab=True)

    if not regions:
        print(f"Warning: no regions found for config "
              f"'{conf_node.name}'")
        return

    # Hash the concatenated regions
    h = hashlib.new(hash_name)
    for off, sz in regions:
        h.update(dtb_data[off:off + sz])
    hash_digest = h.digest()

    # Sign the hash
    try:
        sig = _rsa_sign_pkcs1v15(priv_key, hash_name, hash_digest)
    except Exception as exc:
        print(f"Warning: RSA sign failed for config "
              f"'{conf_node.name}': {exc}")
        return

    # Write signature properties
    sig_node.set_property("value", sig)
    sig_node.set_property("signer-name", "mkimage")
    sig_node.set_property("signer-version", "1.0.0")
    ts = int(time.time())
    sig_node.set_property("timestamp", ts)

    # Write hashed-nodes (NUL-separated paths)
    hashed_nodes_bytes = b'\x00'.join(
        p.encode('ascii') for p in hashed_nodes) + b'\x00'
    sig_node.set_property("hashed-nodes", hashed_nodes_bytes)

    # Write hashed-strings: [0, strings_size] as two big-endian u32
    hashed_strings_data = struct.pack(">II", 0, strings_sz)
    sig_node.set_property("hashed-strings", hashed_strings_data)


def _collect_config_hash_paths(
    fdt_obj: fdt.FDT,
    conf_node,
    sign_types: List[str],
) -> List[str]:
    """Collect the DTB node paths that should be hashed for a
    configuration signature.

    For each image type in *sign_types*, find the image names
    referenced in the conf node, and build their full paths under
    ``/images/``.
    """
    paths = []

    # Always include the configuration node itself
    conf_path = f"/configurations/{conf_node.name}"
    paths.append(conf_path)

    # For each sign type, find referenced images
    images_node = fdt_obj.get_node("images")
    if images_node is None:
        return paths

    for img_type in sign_types:
        # Get the image name(s) from the conf node
        prop = conf_node.get_property(img_type)
        if prop is None:
            continue
        # Property value can be a string list
        if isinstance(prop, fdt.PropStrings):
            img_names = list(prop.data)
        else:
            continue

        for img_name in img_names:
            img_name = img_name.strip()
            img_path = f"/images/{img_name}"
            if fdt_obj.exist_node(img_path):
                paths.append(img_path)

    return paths


# ── Public-Key Injection into DTB ──────────────────────────────────

def inject_pubkeys(
    keydir: str,
    keydest: str,
    require_keys: bool = False,
) -> None:
    """Inject RSA public keys into a target DTB file.

    Reads the target DTB, creates/updates a ``/signature`` node
    with public-key information for each key found in *keydir*.

    Corresponds to the C ``rsa_add_verify_data()`` function.
    """
    from Cryptodome.PublicKey import RSA

    with open(keydest, 'rb') as fh:
        dtb_data = fh.read()

    fdt_obj = fdt.parse_dtb(dtb_data)

    # Create /signature node if it doesn't exist
    sig_node = fdt_obj.get_node("signature", create=True)

    # Scan keydir for .key files
    if not os.path.isdir(keydir):
        print(f"Warning: keydir '{keydir}' is not a directory")
        return

    for filename in os.listdir(keydir):
        if not filename.endswith('.key'):
            continue
        key_name = filename[:-4]
        key_path = os.path.join(keydir, filename)

        try:
            with open(key_path, 'rb') as fh:
                priv_key = RSA.import_key(fh.read())
        except Exception as exc:
            print(f"Warning: failed to load key '{filename}': {exc}")
            continue

        pub_key = priv_key.public_key()

        # Create a sub-node for this key
        key_node = fdt.Node(key_name)

        # Write public key properties
        n = pub_key.n
        e = pub_key.e
        key_bits = n.bit_length()

        # Convert n to big-endian bytes
        n_bytes = n.to_bytes(
            (n.bit_length() + 7) // 8, byteorder='big')
        # Convert exponent to big-endian bytes
        e_bytes = e.to_bytes(
            (e.bit_length() + 7) // 8, byteorder='big')

        key_node.set_property("algo", f"sha256,rsa{key_bits}")
        key_node.set_property("rsa,num-bits", key_bits)
        key_node.set_property("rsa,modulus", n_bytes)
        key_node.set_property("rsa,exponent", e_bytes)

        # Compute n0 (Montgomery constant): -1/n mod 2^32
        # This is needed by the U-Boot RSA verification code
        n0_inv = pow(n, -1, 1 << 32)
        n0 = (1 << 32) - n0_inv
        key_node.set_property("rsa,n0-inverse", n0 & 0xFFFFFFFF)

        if require_keys:
            key_node.set_property("required", "image")

        sig_node.append(key_node)

    # Write back the modified DTB
    new_dtb = fdt_obj.to_dtb(version=17)
    with open(keydest, 'wb') as fh:
        fh.write(new_dtb)
