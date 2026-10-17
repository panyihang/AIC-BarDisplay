#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2023-2026 ArtInChip Technology Co., Ltd
# Dehuang Wu <dehuang.wu@artinchip.com>

import os
import sys
import re
import subprocess
import json
import argparse
from collections import namedtuple
from collections import OrderedDict

VERBOSE = False


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
    if "-" in size_str:
        return 0
    return int(size_str, 10)


def _is_legacy_two_flash(cfg, device_name):
    """Check if this is a legacy two-flash configuration

    Args:
        cfg: Configuration dictionary
        device_name: Device name to check

    Returns:
        bool: True if this is a legacy two-flash configuration
    """
    return "device_count" in cfg.get(device_name, {})


def _handle_legacy_two_flash_file_list(cfg, device_name):
    """Handle legacy two-flash configuration for file list generation

    Args:
        cfg: Configuration dictionary
        device_name: Primary device name

    Returns:
        str: Combined partition file list string for both flashes
    """
    # Handle primary flash
    part_str_segment = aic_create_part_file_string(cfg, device_name, 0)

    # Handle secondary flash (device_type1, size1, partitions1)
    if "device_type1" not in cfg[device_name]:
        return part_str_segment

    # Get secondary flash info from media1
    media1_cfg = cfg["image"]["info"].get("media1", {})
    if not media1_cfg:
        return part_str_segment

    # Determine secondary device name and type
    device_name1 = media1_cfg.get("type", cfg[device_name].get("device_type1", "unknown"))

    # Create a temporary device config for the second flash
    cfg[device_name1] = {
        "size": cfg[device_name]["size1"],
        "partitions": cfg[device_name]["partitions1"]
    }

    # Determine media type for second flash
    if device_name1 in ["spi-nand", "spi-nor", "mmc"]:
        mt = device_name1
    else:
        # Try to infer from device_name
        if "nand" in device_name1.lower():
            mt = "spi-nand"
        elif "nor" in device_name1.lower():
            mt = "spi-nor"
        else:
            mt = "spi-nand"  # default

    # Generate file list for secondary flash
    if mt == "spi-nand":
        part_str_segment += aic_create_nand_part_file_string(cfg, device_name1, 0)
    else:
        part_str_segment += aic_create_part_file_string(cfg, device_name1, 0)

    return part_str_segment


def _extract_media_info(cfg):
    """Extract and normalize media info from config (support both old and new format)

    Args:
        cfg: Configuration dictionary

    Returns:
        list: List of dicts with keys: "name", "type", "controller"

    Old format: {"type": "spi-nor", "device_id": 0}
    New format: {"name": ["spi-nor0", "spi-nand1"], "controller": [0, 1]}
    Legacy format: {"type": "spi-nor", "device_id": 0} with device_count in device config
    """
    media_cfg = cfg["image"]["info"]["media"]
    media_list = []

    if "type" in media_cfg:
        # Old format or legacy format: type and device_id
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
                "name": mt,          # Use type as name in old format
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


def aic_create_part_file_string(cfg, device_name, start_offs):
    part_str = ""
    part_size = 0
    part_offs = start_offs
    total_siz = 0
    total_siz = size_str_to_int(cfg[device_name]["size"])
    partitions = cfg[device_name]["partitions"]
    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)
    for part in partitions:
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
        part_offs += part_size
        if "offset" in partitions[part]:
            part_offs = size_str_to_int(partitions[part]["offset"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        else:
            part_size = size_str_to_int(partitions[part]["size"])
        partitions[part]["part_size"] = part_size
        partitions[part]["part_offs"] = part_offs

    target = cfg["image"]["target"]
    for name in target:
        fwc = target[name]
        part_list = fwc["part"]
        for idx in range(len(part_list)):
            part = part_list[idx]
            if part not in partitions.keys():
                # Skip partitions not in this device
                continue
            part_str += "{},{},{}\n".format(part, fwc["file"], partitions[part]["part_size"])
    return part_str


def aic_create_nand_part_file_string(cfg, device_name, start_offs):
    part_str = ""
    part_size = 0
    part_offs = start_offs
    total_siz = 0
    organization = cfg["image"]["info"]["media"].get("array_organization", [])
    total_siz = size_str_to_int(cfg[device_name]["size"])
    partitions = cfg[device_name]["partitions"]

    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)
    nands = ""
    if len(organization):
        nands = "nands="
    for idx in range(len(organization)):
        pagesiz = 0
        if "page" in organization[idx]:
            pagesiz = size_str_to_int(organization[idx]["page"])
        blocksiz = 0
        if "block" in organization[idx]:
            blocksiz = size_str_to_int(organization[idx]["block"])
        oobsiz = 0
        if "oob" in organization[idx]:
            oobsiz = size_str_to_int(organization[idx]["oob"])
        nands += "{},{},{};".format(pagesiz, blocksiz, oobsiz)
    if len(nands) > 0:
        part_str += nands[0:-1]
        part_str += "\n"
    for part in partitions:
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
        part_offs += part_size
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            part_offs = size_str_to_int(partitions[part]["offset"])
        partitions[part]["part_size"] = part_size
        partitions[part]["part_offs"] = part_offs

    target = cfg["image"]["target"]
    for name in target:
        fwc = target[name]
        part_list = fwc["part"]
        for idx in range(len(part_list)):
            part = part_list[idx]
            if part not in partitions.keys():
                # Skip partitions not in this device
                continue
            part_str += "{},{},{}\n".format(part, fwc["file"], partitions[part]["part_size"])
    return part_str


def aic_create_parts_string(cfg):
    # Extract media info (support both old and new format)
    media_list = _extract_media_info(cfg)

    all_part_strs = []

    for media in media_list:
        device_name = media["name"]
        media_type = media["type"]

        # Check if this is a legacy two-flash configuration
        if _is_legacy_two_flash(cfg, device_name):
            # Legacy config: handle both primary and secondary flash
            part_str_segment = _handle_legacy_two_flash_file_list(cfg, device_name)
            all_part_strs.append(part_str_segment)
        else:
            # Normal config (old or new format)
            part_str_segment = ""
            if media_type == "spi-nor":
                part_str_segment = aic_create_part_file_string(cfg, device_name, 0)
            elif media_type == "spi-nand":
                part_str_segment = aic_create_nand_part_file_string(cfg, device_name, 0)
            elif media_type == "mmc":
                part_str_segment = aic_create_part_file_string(cfg, device_name, 17*1024)

            all_part_strs.append(part_str_segment)

    # Each segment already ends with '\n', use empty string join to avoid double newlines
    return ''.join(all_part_strs)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output partition file")
    args = parser.parse_args()
    if args.config is None:
        print('Error, option --config is required.')
        sys.exit(1)

    cfg = parse_image_cfg(args.config)
    parts = aic_create_parts_string(cfg)
    if args.outfile is None:
        print(parts)
    else:
        with open(args.outfile, "w+") as f:
            f.write(parts)
            f.close()
