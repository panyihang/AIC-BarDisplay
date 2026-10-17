# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2021-2026 ArtInChip Technology Co., Ltd
#
# Pure Python private resource data builder.
# Only uses Python 3.7+ standard library.
"""
aic_private_resource — internal implementation.

Public API (re-exported from ``__init__.py``):
    parse_config()    — load JSON config with JS-style comment support
    build_private_data() — generate binary private resource from config dict
    build_from_files()   — convenience: parse configs → build → write
    main()            — CLI entry point
"""
import argparse
import json
import os
import re
import struct
import sys
from collections import OrderedDict
from typing import Any, Dict, List, Optional, Sequence

# ── Section type constants ─────────────────────────────────────────
SECT_DRAM: int = 0x41490001
SECT_UART: int = 0x41490002
SECT_JTAG: int = 0x41490003
SECT_UPGMODE: int = 0x41490004
SECT_PART: int = 0x41490005
SECT_PSRAM: int = 0x41490006
SECT_REGCFG: int = 0x41490007
SECT_SECURITY_KEYDATA: int = 0x41490008
SECT_END: int = 0x4149FFFF

# ── Default fill value for optional fields ─────────────────────────
_U32_FILL: str = "0xFFFFFFFF"

# ── Field specifications (ordered, matching C struct layout) ───────

"""
struct ddr {
    u32 ddr_type   ;
    u32 ddr_size   ;
    u32 ddr_freq   ;
    u32 ddr_zq     ;
    u32 ddr_odt_en ;
    u32 ddr_para1  ;
    u32 ddr_para2  ;
    u32 ddr_mr0    ;
    u32 ddr_mr1    ;
    u32 ddr_mr2    ;
    u32 ddr_mr3    ;
    u32 ddr_mr4    ;
    u32 ddr_mr5    ;
    u32 ddr_mr6    ;
    u32 ddr_tpr0   ;
    u32 ddr_tpr1   ;
    u32 ddr_tpr2   ;
    u32 ddr_tpr3   ;
    u32 ddr_tpr4   ;
    u32 ddr_tpr5   ;
    u32 ddr_tpr6   ;
    u32 ddr_tpr7   ;
    u32 ddr_tpr8   ;
    u32 ddr_tpr9   ;
    u32 ddr_tpr10  ;
    u32 ddr_tpr11  ;
    u32 ddr_tpr12  ;
    u32 ddr_tpr13  ;
    u32 ddr_tpr14  ;
    u32 ddr_tpr15  ;
    u32 ddr_tpr16  ;
    u32 ddr_tpr17  ;
    u32 ddr_tpr18  ;
};
struct dram_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    u32 entry_cnt;
    struct ddr param[entry_cnt];
};
"""

# struct ddr — 29 × u32
_DDR_FIELDS: List[str] = [
    "type", "memsize", "freq", "zq", "odt", "para1", "para2",
    "mr0", "mr1", "mr2", "mr3", "mr4", "mr5", "mr6",
    "tpr0", "tpr1", "tpr2", "tpr3", "tpr4", "tpr5", "tpr6",
    "tpr7", "tpr8", "tpr9", "tpr10", "tpr11", "tpr12", "tpr13",
    "tpr14", "tpr15", "tpr16", "tpr17", "tpr18",
]

"""
struct psram {
    struct {
        u32 clock;
        u32 cs0_pins;
        u32 cs1_pins;
        u32 xspi_ctl;
        u32 xspi_tcr;
        u32 xspi_cfg;
        u32 xspi_ldo;
        u32 psram_cfg0;
        u32 psram_cfg1;
        u32 xspi_cs0_iocfg1;
        u32 xspi_cs0_iocfg2;
        u32 xspi_cs0_iocfg3;
        u32 xspi_cs0_iocfg4;
        u32 xspi_cs1_iocfg1;
        u32 xspi_cs1_iocfg2;
        u32 xspi_cs1_iocfg3;
        u32 xspi_cs1_iocfg4;
    } common;
    struct {
        u32 proto;
        u32 buf;
    } reset;
    struct {
        u32 proto;
        u32 id;
        u32 buf;
    } getid;
    struct {
        u32 proto0;
        u32 buf0;
        u32 proto1;
        u32 buf1;
        u32 proto2;
        u32 buf2;
        u32 proto3;
        u32 buf3;
    } init;
    struct {
        u32 wr_proto;
        u32 wr_buf;
        u32 rd_proto;
        u32 rd_buf;
    } xip_cfg;
    struct {
        u32 buf0;
        u32 buf1;
        u32 buf2;
        u32 buf3;
        u32 buf4;
        u32 buf5;
        u32 buf6;
        u32 buf7;
        u32 buf8;
        u32 buf9;
    } backup;
};
struct psram_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    u32 entry_cnt;
    struct psram param[entry_cnt];
};
"""

# struct psram.common — 17 × u32
_PSRAM_COMMON_FIELDS: List[str] = [
    "clock", "cs0_pins", "cs1_pins",
    "xspi_ctl", "xspi_tcr", "xspi_cfg", "xspi_ldo",
    "psram_cfg0", "psram_cfg1",
    "xspi_cs0_iocfg1", "xspi_cs0_iocfg2",
    "xspi_cs0_iocfg3", "xspi_cs0_iocfg4",
    "xspi_cs1_iocfg1", "xspi_cs1_iocfg2",
    "xspi_cs1_iocfg3", "xspi_cs1_iocfg4",
]

# struct psram.reset — 2 × u32
_PSRAM_RESET_FIELDS: List[str] = ["proto", "buf"]

# struct psram.getid — 3 × u32
_PSRAM_GETID_FIELDS: List[str] = ["proto", "id", "buf"]

# struct psram.init — 8 × u32
_PSRAM_INIT_FIELDS: List[str] = [
    "proto0", "buf0", "proto1", "buf1",
    "proto2", "buf2", "proto3", "buf3",
]

# struct psram.xip_cfg — 4 × u32
_PSRAM_XIP_FIELDS: List[str] = ["wr_proto", "wr_buf", "rd_proto", "rd_buf"]

# struct psram.backup — 10 × u32
_PSRAM_BACKUP_FIELDS: List[str] = [
    "buf0", "buf1", "buf2", "buf3", "buf4",
    "buf5", "buf6", "buf7", "buf8", "buf9",
]

"""
struct system_uart {
    u32 uart_id;
    u32 uart_tx_pin_cfg_reg;
    u32 uart_tx_pin_cfg_val;
    u32 uart_rx_pin_cfg_reg;
    u32 uart_rx_pin_cfg_val;
};
struct system_uart_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    struct system_uart param[entry_cnt];
};
"""

# struct system_uart — 5 × u32
_UART_FIELDS: List[str] = [
    "uart_id",
    "uart_tx_pin_cfg_reg", "uart_tx_pin_cfg_val",
    "uart_rx_pin_cfg_reg", "uart_rx_pin_cfg_val",
]

"""
struct system_jtag {
    u32 jtag_id;
    u32 uart_do_pin_cfg_reg;
    u32 uart_do_pin_cfg_val;
    u32 uart_di_pin_cfg_reg;
    u32 uart_di_pin_cfg_val;
    u32 uart_ms_pin_cfg_reg;
    u32 uart_ms_pin_cfg_val;
    u32 uart_ck_pin_cfg_reg;
    u32 uart_ck_pin_cfg_val;
};
struct system_jtag_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    u32 jtag_only;
    struct system_jtag param[entry_cnt];
};
"""

# struct system_jtag — 8 × u32
_JTAG_FIELDS: List[str] = [
    "jtag_id",
    "jtag_do_pin_cfg_reg", "jtag_do_pin_cfg_val",
    "jtag_di_pin_cfg_reg", "jtag_di_pin_cfg_val",
    "jtag_ms_pin_cfg_reg", "jtag_ms_pin_cfg_val",
    "jtag_ck_pin_cfg_reg", "jtag_ck_pin_cfg_val",
]

"""
struct system_reg_cfg {
    u32 reg;
    u32 val;
    u32 dly;
};
struct system_reg_cfg_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    u32 entry_cnt;
    struct system_reg_cfg param[entry_cnt];
};
"""

# struct system_reg_cfg — 3 × u32
_REGCFG_FIELDS: List[str] = ["reg", "val", "dly"]

"""
struct system_upgmode {
    u32 upgmode_pin_cfg_reg;
    u32 upgmode_pin_cfg_val;
    u32 upgmode_pin_input_reg;
    u32 upgmode_pin_input_msk;
    u32 upgmode_pin_input_val;
    u32 upgmode_pin_pullup_dly;
};
struct system_upgmode_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    struct system_upgmode;
};
"""

# struct system_upgmode — 6 × u32
_UPGMODE_FIELDS: List[str] = [
    "upgmode_pin_cfg_reg", "upgmode_pin_cfg_val",
    "upgmode_pin_input_reg", "upgmode_pin_input_msk",
    "upgmode_pin_input_val", "upgmode_pin_pullup_dly",
]


"""
struct security_keydata {
    u32 data_type;
    u32 data_len; // length of reset of this structure
    u32 version;
    u32 data[]
}
"""


# ── Utility functions ──────────────────────────────────────────────

def _parse_int(s: str) -> int:
    """Parse a decimal or hexadecimal string to int."""
    return int(s, 16) if "0x" in s or "0X" in s else int(s, 10)


def _u32(s: str) -> bytes:
    """Encode a numeric string as a little-endian u32."""
    return struct.pack("<I", _parse_int(s))


def _field(cfg: Dict[str, str], name: str, default: str = "0") -> bytes:
    """Read a u32 field from config dict, with fallback default."""
    return _u32(cfg.get(name, default))


def _encode_fields(cfg: Dict[str, str], fields: Sequence[str],
                   default: str = "0") -> bytes:
    """Encode multiple u32 fields in order."""
    return b"".join(_field(cfg, f, default) for f in fields)


def _pack_section(sect_type: int, payload: bytes) -> bytes:
    """Build a section: [type:u32][data_len:u32][payload]."""
    return struct.pack("<II", sect_type, len(payload)) + payload


# ── Config parser ──────────────────────────────────────────────────

def parse_config(path: str) -> Dict[str, Any]:
    """Parse a JSON config file with JS-style ``//`` comment support.

    Trailing commas before ``}`` or ``]`` are stripped automatically.
    Uses :class:`~collections.OrderedDict` to preserve iteration order
    (required for correct firmware component ordering).
    """
    with open(path, "r") as f:
        text = f.read()
    # Strip full-line comments
    text = re.sub(r"^\s*//.*$", "", text, flags=re.MULTILINE)
    # Strip inline comments
    text = re.sub(r"//.*$", "", text, flags=re.MULTILINE)
    # Fix trailing commas (with optional whitespace before } or ])
    text = re.sub(r",\s*}", "}", text)
    text = re.sub(r",\s*\]", "]", text)
    return json.loads(text, object_pairs_hook=OrderedDict)


# ── Section builders ───────────────────────────────────────────────

def _build_dram(dram: Dict[str, Any]) -> bytes:
    """
    struct dram_data {
        u32 data_type; u32 data_len; u32 entry_cnt;
        struct ddr param[entry_cnt];  // 29 × u32 each
    };
    """
    chunks: List[bytes] = [struct.pack("<I", len(dram))]
    for entry in dram.values():
        chunks.append(_encode_fields(entry, _DDR_FIELDS))
    return _pack_section(SECT_DRAM, b"".join(chunks))


def _build_psram(psram: Dict[str, Any]) -> bytes:
    """
    struct psram_data {
        u32 data_type; u32 data_len; u32 entry_cnt;
        struct psram param[entry_cnt];
    };
    """
    chunks: List[bytes] = [struct.pack("<I", len(psram))]
    for entry in psram.values():
        chunks.append(_encode_fields(entry["common"], _PSRAM_COMMON_FIELDS))
        chunks.append(_encode_fields(entry["reset"], _PSRAM_RESET_FIELDS, _U32_FILL))
        chunks.append(_encode_fields(entry["getid"], _PSRAM_GETID_FIELDS, _U32_FILL))
        chunks.append(_encode_fields(entry["init"], _PSRAM_INIT_FIELDS, _U32_FILL))
        chunks.append(_encode_fields(entry["xip_cfg"], _PSRAM_XIP_FIELDS, _U32_FILL))
        chunks.append(_encode_fields(entry["backup"], _PSRAM_BACKUP_FIELDS, _U32_FILL))
    return _pack_section(SECT_PSRAM, b"".join(chunks))


def _build_system_uart(sys_uart: Dict[str, Any]) -> bytes:
    """
    struct system_uart_data {
        u32 data_type; u32 data_len;
        struct system_uart param[entry_cnt];  // 5 × u32 each
    };
    """
    chunks: List[bytes] = []
    for entry in sys_uart.values():
        chunks.append(_encode_fields(entry, _UART_FIELDS))
    return _pack_section(SECT_UART, b"".join(chunks))


def _build_system_jtag(sys_jtag: Dict[str, Any]) -> bytes:
    """
    struct system_jtag_data {
        u32 data_type; u32 data_len; u32 jtag_only;
        struct system_jtag param[entry_cnt];  // 8 × u32 each
    };
    """
    chunks: List[bytes] = [_u32(sys_jtag["jtag_only"])]
    for val in sys_jtag.values():
        if isinstance(val, OrderedDict):
            chunks.append(_encode_fields(val, _JTAG_FIELDS))
    return _pack_section(SECT_JTAG, b"".join(chunks))


def _build_system_regcfg(cfgs: Dict[str, Any]) -> bytes:
    """
    struct system_reg_cfg_data {
        u32 data_type; u32 data_len; u32 entry_cnt;
        struct system_reg_cfg param[entry_cnt];  // 3 × u32 each
    };
    """
    count = min(_parse_int(cfgs["count"]), len(cfgs["regs"]))
    chunks: List[bytes] = [struct.pack("<I", count)]
    for i in range(count):
        regi = cfgs["regs"][i]
        if isinstance(regi, OrderedDict):
            chunks.append(_encode_fields(regi, _REGCFG_FIELDS))
    return _pack_section(SECT_REGCFG, b"".join(chunks))


def _build_system_upgmode(sys_upgmode: Dict[str, str]) -> bytes:
    """
    struct system_upgmode_data {
        u32 data_type; u32 data_len;
        struct system_upgmode;  // 6 × u32
    };
    """
    data = _encode_fields(sys_upgmode, _UPGMODE_FIELDS[:-1])
    # upgmode_pin_pullup_dly has a special default of "500"
    data += _field(sys_upgmode, _UPGMODE_FIELDS[-1], "500")
    return _pack_section(SECT_UPGMODE, data)


"""
struct partition_data {
    u32 data_type;
    u32 data_len; // length of rest of this structure
    u8  part_str[];
};
"""


def _build_partition(parts: Dict[str, Any]) -> bytes:
    """
    struct partition_data {
        u32 data_type; u32 data_len;
        u8  part_str[];  // 4-byte aligned
    };
    """
    part_str = ""
    if "type" in parts:
        for t in parts["type"]:
            if t in parts:
                part_str += f"{t}={parts[t]};"
    raw = part_str.encode("utf-8")
    # Pad to 4-byte alignment (always add padding, matching original behavior)
    pad_len = 4
    remainder = len(raw) % 4
    if remainder:
        pad_len = 4 - remainder
    raw += b"\x00" * pad_len
    return _pack_section(SECT_PART, raw)


def _build_security_keydata(sec: Dict[str, Any], config_dir: str = ".") -> bytes:
    """
    struct security_keydata {
        u32 data_type;
        u32 data_len; // length of rest of this structure
        u32 version;
        u32 data[];
    };
    """
    keydata = sec["keydata"]
    version = _parse_int(keydata.get("version", "1"))

    # Read the binary file referenced by "file" field
    file_name = keydata["file"]
    file_path = os.path.join(config_dir, file_name)
    with open(file_path, "rb") as f:
        file_data = f.read()

    # Pad file data to u32 (4-byte) alignment
    remainder = len(file_data) % 4
    if remainder:
        file_data += b"\x00" * (4 - remainder)

    # Payload: version(u32) + data[]
    payload = struct.pack("<I", version) + file_data
    return _pack_section(SECT_SECURITY_KEYDATA, payload)


def _build_end() -> bytes:
    """End-of-data marker section."""
    return struct.pack("<II", SECT_END, 0)


# ── Top-level builder ──────────────────────────────────────────────

# Dispatch table: config key → builder function
# Note: "security" is handled separately because it needs config_dir
_SECTION_BUILDERS = {
    "dram":       _build_dram,
    "psram":      _build_psram,
    "partitions": _build_partition,
}

_SYSTEM_BUILDERS = {
    "upgmode": _build_system_upgmode,
    "uart":    _build_system_uart,
    "jtag":    _build_system_jtag,
    "regcfg":  _build_system_regcfg,
}


def build_private_data(cfg: Dict[str, Any], config_dir: str = ".") -> bytes:
    """Generate binary private resource data from a merged config dict.

    Iterates config keys in insertion order, dispatching to the
    appropriate section builder for each recognised section type.
    Appends an end-of-data marker at the end.

    Args:
        cfg:        Merged config dict (from parse_config).
        config_dir: Base directory for resolving file references
                    (e.g. security keydata files).
    """
    chunks: List[bytes] = []
    for key in cfg:
        if key.startswith("_"):
            continue  # skip internal keys like _config_dir
        if key in _SECTION_BUILDERS:
            chunks.append(_SECTION_BUILDERS[key](cfg[key]))
        elif key == "security":
            chunks.append(_build_security_keydata(cfg[key], config_dir))
        elif key == "system":
            for sys_key in cfg["system"]:
                builder = _SYSTEM_BUILDERS.get(sys_key)
                if builder:
                    chunks.append(builder(cfg["system"][sys_key]))
    chunks.append(_build_end())
    return b"".join(chunks)


def build_from_files(config_files: Sequence[str],
                     output_path: str) -> bytes:
    """Parse one or more config files, merge them, build binary, and write.

    Args:
        config_files: Paths to JSON config files (merged in order).
        output_path:  Where to write the resulting binary.

    Returns:
        The generated binary data.
    """
    cfg: Dict[str, Any] = {}
    for path in config_files:
        cfg.update(parse_config(path))
    # Use first config file's directory as base for file references
    if config_files:
        config_dir = os.path.dirname(os.path.abspath(config_files[0]))
    else:
        config_dir = "."
    data = build_private_data(cfg, config_dir)
    with open(output_path, "wb") as f:
        f.write(data)
    return data


# ── CLI entry point ────────────────────────────────────────────────

def main() -> None:
    """CLI entry point."""
    parser = argparse.ArgumentParser(
        description="Generate AIC private resource binary from JSON config")
    parser.add_argument(
        "-c", "--config", type=str,
        help="resource private data configuration file name")
    parser.add_argument(
        "-l", "--config_list", type=lambda s: s.split(","),
        help="resource private data configuration file list, "
             "concat with commas")
    parser.add_argument(
        "-o", "--output", type=str,
        help="output file name")
    parser.add_argument(
        "-v", "--verbose", action="store_true",
        help="show detail information")
    args = parser.parse_args()

    if args.config is None and args.config_list is None:
        print("Error, option --config or --config_list is required.")
        sys.exit(1)
    if args.output is None:
        if args.config is not None:
            args.output = os.path.splitext(args.config)[0] + ".bin"
        else:
            args.output = os.path.splitext(args.config_list[0])[0] + ".bin"

    config_files: List[str] = []
    if args.config is not None:
        config_files.append(args.config)
    if args.config_list is not None:
        config_files.extend(args.config_list)

    data = build_from_files(config_files, args.output)
    if args.verbose:
        print(f"Generated {args.output}: {len(data)} bytes")
