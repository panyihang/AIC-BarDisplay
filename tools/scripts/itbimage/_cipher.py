# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
"""
itbimage._cipher — AES-CBC and ChaCha20 encryption for FIT images.

Corresponds to C ``fit_image_process_cipher()`` in
``u-boot/tools/image-host.c``.

Supported algorithms:
  - ``aes128`` / ``aes192`` / ``aes256`` — AES-CBC with PKCS7 padding
  - ``chacha20`` — ChaCha20 stream cipher (no padding)
"""
import os
import struct
from typing import Optional

import fdt


# ── Algorithm Configuration ──────────────────────────────────────────

_AES_KEY_SIZES = {
    'aes128': 16,
    'aes192': 24,
    'aes256': 32,
}

_CHACHA20_KEY_SIZE = 32
_CHACHA20_NONCE_SIZE = 12


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


def _read_binary_file(keydir: str, name: str) -> bytes:
    """Read a binary file from ``{keydir}/{name}.bin``."""
    path = os.path.join(keydir, name + '.bin')
    with open(path, 'rb') as fh:
        return fh.read()


def _pkcs7_pad(data: bytes, block_size: int = 16) -> bytes:
    """Apply PKCS7 padding."""
    pad_len = block_size - (len(data) % block_size)
    return data + bytes([pad_len] * pad_len)


# ── AES-CBC Encryption ─────────────────────────────────────────────

def _encrypt_aes_cbc(
    data: bytes,
    key: bytes,
    iv: bytes,
) -> bytes:
    """Encrypt *data* with AES-CBC and PKCS7 padding."""
    from Cryptodome.Cipher import AES

    cipher = AES.new(key, AES.MODE_CBC, iv=iv)
    padded = _pkcs7_pad(data, AES.block_size)
    return cipher.encrypt(padded)


# ── ChaCha20 Encryption ────────────────────────────────────────────

def _encrypt_chacha20(
    data: bytes,
    key: bytes,
    nonce: bytes,
) -> bytes:
    """Encrypt *data* with ChaCha20 (stream cipher, no padding)."""
    from Cryptodome.Cipher import ChaCha20

    cipher = ChaCha20.new(key=key, nonce=nonce)
    return cipher.encrypt(data)


# ── Public API ──────────────────────────────────────────────────────

def cipher_images(fdt_obj: fdt.FDT, keydir: str) -> None:
    """Encrypt image data for nodes that contain a ``cipher``
    sub-node.

    Corresponds to C ``fit_image_cipher_data()`` →
    ``fit_image_process_cipher()``.

    For each image node under ``/images``, looks for a child node
    named ``cipher``.  If found, reads the ``algo``,
    ``key-name-hint``, and optional ``iv-name-hint`` properties,
    loads the key/IV from *keydir*, encrypts the data, and replaces
    the ``data`` property with the ciphertext.
    """
    images_node = fdt_obj.get_node("images")
    if images_node is None:
        return

    for img_node in list(images_node.nodes):
        # Find cipher sub-node
        cipher_node = None
        for sub in img_node.nodes:
            if sub.name == "cipher" or sub.name.startswith("cipher"):
                cipher_node = sub
                break
        if cipher_node is None:
            continue

        data = _get_image_data(img_node)
        if not data:
            continue

        algo = _get_prop_str(cipher_node, "algo").lower()
        key_hint = _get_prop_str(cipher_node, "key-name-hint")
        iv_hint = _get_prop_str(cipher_node, "iv-name-hint")

        if not algo or not key_hint:
            print(f"Warning: cipher node in '{img_node.name}' "
                  f"missing algo or key-name-hint")
            continue

        try:
            ciphertext, iv_used = _encrypt_image(
                algo, data, keydir, key_hint, iv_hint)
        except Exception as exc:
            print(f"Warning: encryption failed for "
                  f"'{img_node.name}': {exc}")
            continue

        # Replace data with ciphertext
        img_node.set_property("data", ciphertext)
        # Record original (unciphered) size
        img_node.set_property("data-size-unciphered", len(data))

        # If no iv-name-hint, write the generated IV into the
        # cipher node so the decryptor can retrieve it
        if not iv_hint and iv_used:
            cipher_node.set_property("iv", iv_used)


def _encrypt_image(
    algo: str,
    data: bytes,
    keydir: str,
    key_hint: str,
    iv_hint: str,
):
    """Encrypt *data* with the specified algorithm.

    Returns ``(ciphertext, iv_or_nonce)``.
    The second element is the IV/nonce actually used (needed when
    *iv_hint* is empty, so it can be stored in the FIT).
    """
    if algo in _AES_KEY_SIZES:
        key_size = _AES_KEY_SIZES[algo]
        key = _read_binary_file(keydir, key_hint)
        if len(key) < key_size:
            raise ValueError(
                f"AES key '{key_hint}.bin' too short "
                f"({len(key)} < {key_size})")
        key = key[:key_size]

        if iv_hint:
            iv = _read_binary_file(keydir, iv_hint)
            if len(iv) < 16:
                raise ValueError(
                    f"AES IV '{iv_hint}.bin' too short "
                    f"({len(iv)} < 16)")
            iv = iv[:16]
        else:
            iv = os.urandom(16)

        ciphertext = _encrypt_aes_cbc(data, key, iv)
        return ciphertext, iv

    elif algo == 'chacha20':
        key = _read_binary_file(keydir, key_hint)
        if len(key) < _CHACHA20_KEY_SIZE:
            raise ValueError(
                f"ChaCha20 key '{key_hint}.bin' too short "
                f"({len(key)} < {_CHACHA20_KEY_SIZE})")
        key = key[:_CHACHA20_KEY_SIZE]

        if iv_hint:
            nonce = _read_binary_file(keydir, iv_hint)
            if len(nonce) < _CHACHA20_NONCE_SIZE:
                raise ValueError(
                    f"ChaCha20 nonce '{iv_hint}.bin' too short "
                    f"({len(nonce)} < {_CHACHA20_NONCE_SIZE})")
            nonce = nonce[:_CHACHA20_NONCE_SIZE]
        else:
            nonce = os.urandom(_CHACHA20_NONCE_SIZE)

        ciphertext = _encrypt_chacha20(data, key, nonce)
        return ciphertext, nonce

    else:
        raise ValueError(f"Unsupported cipher algorithm: {algo}")
