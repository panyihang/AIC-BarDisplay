#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2022-2026, ArtInChip Technology Co., Ltd
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
    try:
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
    except FileNotFoundError:
        print("Error: Configuration file not found: {}".format(cfgfile))
        sys.exit(1)
    except PermissionError:
        print("Error: Permission denied to read: {}".format(cfgfile))
        sys.exit(1)
    except json.JSONDecodeError as e:
        print("Error: Invalid JSON format in {}: {}".format(cfgfile, e))
        sys.exit(1)


def size_str_to_int(size_str):
    if "k" in size_str or "K" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        if not numstr:
            print("Error: Invalid size string: {}".format(size_str))
            sys.exit(1)
        return (int(numstr) * 1024)
    if "m" in size_str or "M" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        if not numstr:
            print("Error: Invalid size string: {}".format(size_str))
            sys.exit(1)
        return (int(numstr) * 1024 * 1024)
    if "g" in size_str or "G" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        if not numstr:
            print("Error: Invalid size string: {}".format(size_str))
            sys.exit(1)
        return (int(numstr) * 1024 * 1024 * 1024)
    if "0x" in size_str or "0X" in size_str:
        return int(size_str, 16)
    if "-" in size_str:
        return 0
    return int(size_str, 10)


def aic_create_two_flash_parts_json_legacy(cfg, media_type, part_type):
    mtd = ""
    nftl = ""
    part_str = '{\n\t"partitions": {\n'
    part_size = 0
    part_offs = 0
    total_siz = 0

    total_siz = size_str_to_int(cfg[media_type]["size"])
    partitions = cfg[media_type]["partitions"]
    part_type.append("mtd")
    mtd = "spi{}.0:".format(cfg["image"]["info"]["media"]["device_id"])
    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)

    for part in partitions:
        itemstr = ""
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
            sys.exit(1)
        part_offs += part_size
        itemstr += partitions[part]["size"]
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            itemstr += "@{}".format(partitions[part]["offset"])
            part_offs = size_str_to_int(partitions[part]["offset"])
        itemstr += "({})".format(part)

        mtd += itemstr + ","

    # seconed flash
    total_siz = size_str_to_int(cfg[media_type]["size1"])
    partitions = cfg[media_type]["partitions1"]
    media_type1 = cfg["image"]["info"]["media1"]["type"]

    mtd = mtd[0:-1]
    mtd += ";"
    mtd += "spi{}.0:".format(cfg["image"]["info"]["media1"]["device_id"])
    part_size = 0
    part_offs = 0
    for part in partitions:
        itemstr = ""
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
            sys.exit(1)
        part_offs += part_size
        itemstr += partitions[part]["size"]
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            itemstr += "@{}".format(partitions[part]["offset"])
            part_offs = size_str_to_int(partitions[part]["offset"])
        itemstr += "({})".format(part)

        if "nftl" in partitions[part]:
            if "nftl" not in part_type:
                part_type.append("nftl")
            nftl_volumes = partitions[part]["nftl"]
            if len(nftl_volumes) == 0:
                print("Volume of {} is empty".format(part))
                sys.exit(1)
            nftl += "{}:".format(part)
            for vol in nftl_volumes:
                itemstr = ""
                if "size" not in nftl_volumes[vol]:
                    print("No size value for nftl volume: {}".format(vol))
                    sys.exit(1)
                itemstr += nftl_volumes[vol]["size"]
                if "offset" in nftl_volumes[vol]:
                    itemstr += "@{}".format(nftl_volumes[vol]["offset"])
                itemstr += "({})".format(vol)
                nftl += itemstr + ","
            nftl = nftl[0:-1] + ";"

        mtd += itemstr + ","

    mtd = mtd[0:-1]
    # Build JSON fields list to avoid trailing comma
    json_fields = []
    json_fields.append("\t\t\"mtd\" : \"{}\"".format(mtd))
    if len(nftl) > 0:
        nftl = nftl[0:-1]
        json_fields.append("\t\t\"nftl\" : \"{}\"".format(nftl))

    # Deduplicate part_type and add to fields
    unique_part_types = list(dict.fromkeys(part_type))
    json_fields.append('\t\t"type": {}'.format(str(unique_part_types).replace("'", '"')))
    # Join fields with comma and newline
    part_str += ",\n".join(json_fields) + "\n"

    part_str += "\t}\n}"
    return part_str


def _normalize_media_type(media_type):
    """Normalize media_type to a list for uniform processing

    Args:
        media_type: Can be string or list of strings

    Returns:
        list: List of media_type strings
    """
    if isinstance(media_type, str):
        return [media_type]
    return media_type


def _is_legacy_two_flash(cfg, device_name):
    """Check if this is a legacy two-flash configuration

    Args:
        cfg: Configuration dictionary
        device_name: Device name to check

    Returns:
        bool: True if this is a legacy two-flash configuration
    """
    return "device_count" in cfg.get(device_name, {})


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


def _build_partition_item(part_name, part_info, part_offs, total_siz=0):
    """Build partition item string in format: size[@offset](name)"""
    itemstr = ""
    p_size = 0

    if "size" not in part_info:
        print("No size value for partition: {}".format(part_name))
        sys.exit(1)

    itemstr += part_info["size"]
    p_size = size_str_to_int(part_info["size"])

    if part_info["size"] == "-":
        p_size = total_siz - part_offs

    if "offset" in part_info:
        itemstr += "@{}".format(part_info["offset"])
        part_offs = size_str_to_int(part_info["offset"])

    itemstr += "({})".format(part_name)

    return itemstr, p_size, part_offs


def _build_volumes_string(part_name, volumes, volume_type):
    """Build volumes string for ubi/nftl/levelx in format: partname:vol1,vol2;"""
    if len(volumes) == 0:
        print("Volume of {} is empty".format(part_name))
        sys.exit(1)

    vol_str = "{}:".format(part_name)
    for vol in volumes:
        itemstr = ""
        if "size" not in volumes[vol]:
            print("No size value for {} volume: {}".format(volume_type, vol))
            sys.exit(1)

        itemstr += volumes[vol]["size"]
        if "offset" in volumes[vol]:
            itemstr += "@{}".format(volumes[vol]["offset"])
        itemstr += "({})".format(vol)
        vol_str += itemstr + ","

    vol_str = vol_str[0:-1] + ";"
    return vol_str


def aic_create_parts_json(cfg):
    # Internal helper function to process a single media_type
    def _process_single_media(mt, device_name, ctrl_id, mtd, ubi, nftl,
                              levelx, part_type, part_size, part_offs, total_siz):
        """Process a single media_type and return updated partition strings"""
        if "device_count" in cfg.get(device_name, {}):
            # setting for legacy two flashes solution
            part_str_segment = aic_create_two_flash_parts_json_legacy(cfg, mt, part_type)
            return part_str_segment, part_type, mtd, ubi, nftl, levelx

        elif mt == "spi-nand" or mt == "spi-nor":
            total_siz = size_str_to_int(cfg[device_name]["size"])
            partitions = cfg[device_name]["partitions"]
            part_type.append("mtd")
            mtd = "spi{}.0:".format(ctrl_id)
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)

            for part in partitions:
                itemstr, part_size, part_offs = _build_partition_item(
                    part, partitions[part], part_offs, total_siz)
                mtd += itemstr + ","

                # Handle ubi volumes
                if "ubi" in partitions[part]:
                    if "ubi" not in part_type:
                        part_type.append("ubi")
                    ubi += _build_volumes_string(part, partitions[part]["ubi"], "ubi")

                # Handle nftl volumes
                if "nftl" in partitions[part]:
                    if "nftl" not in part_type:
                        part_type.append("nftl")
                    nftl += _build_volumes_string(part, partitions[part]["nftl"], "nftl")

                # Handle levelx volumes
                if "levelx" in partitions[part]:
                    if "levelx" not in part_type:
                        part_type.append("levelx")
                    levelx += _build_volumes_string(part, partitions[part]["levelx"], "levelx")

            mtd = mtd[0:-1]
            part_str_segment = ""
            part_str_segment += "\t\t\"mtd\" : \"{}\",\n".format(mtd)
            if len(ubi) > 0:
                ubi = ubi[0:-1]
                part_str_segment += "\t\t\"ubi\" : \"{}\",\n".format(ubi)
            if len(nftl) > 0:
                nftl = nftl[0:-1]
                part_str_segment += "\t\t\"nftl\" : \"{}\",\n".format(nftl)
            if len(levelx) > 0:
                levelx = levelx[0:-1]
                part_str_segment += "\t\t\"levelx\" : \"{}\",\n".format(levelx)

            return part_str_segment, part_type, mtd, ubi, nftl, levelx

        elif mt == "mmc":
            part_type.append("gpt")
            partitions = cfg[device_name]["partitions"]
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)

            gpt = ""
            for part in partitions:
                itemstr, _, _ = _build_partition_item(part, partitions[part], 0)
                gpt += itemstr + ","
            gpt = gpt[0:-1]
            part_str_segment = "\t\t\"gpt\" : \"{}\",\n".format(gpt)
            return part_str_segment, part_type, mtd, ubi, nftl, levelx

        else:
            print("Not supported media type: {}".format(mt))
            sys.exit(1)

    # Extract media info (support both old and new format)
    media_list = _extract_media_info(cfg)

    all_part_types = []
    all_mtd_strs = []
    all_ubi_strs = []
    all_nftl_strs = []
    all_levelx_strs = []
    all_gpt_strs = []

    for media in media_list:
        device_name = media["name"]
        media_type = media["type"]
        ctrl_id = media["controller"]

        # Check if this is a legacy two-flash configuration
        if _is_legacy_two_flash(cfg, device_name):
            # Legacy config: aic_create_two_flash_parts_json_legacy returns complete JSON
            part_type = []
            part_str_segment = aic_create_two_flash_parts_json_legacy(cfg, media_type, part_type)
            return part_str_segment

        part_str_segment, part_type, mtd, ubi, nftl, levelx = _process_single_media(
            media_type, device_name, ctrl_id, "", "", "", "", [], 0, 0, 0)
        all_part_types.extend(part_type)

        # Extract mtd/gpt strings for merging multiple devices
        if mtd:
            all_mtd_strs.append(mtd)
        if ubi:
            all_ubi_strs.append(ubi)
        if nftl:
            all_nftl_strs.append(nftl)
        if levelx:
            all_levelx_strs.append(levelx)
        # Check for GPT (mmc)
        if '"gpt"' in part_str_segment:
            import re
            gpt_match = re.search(r'"gpt"\s*:\s*"([^"]+)"', part_str_segment)
            if gpt_match:
                all_gpt_strs.append(gpt_match.group(1))

    # Build final JSON string
    part_str = '{\n\t"partitions": {\n'

    # Build JSON fields list to avoid trailing comma
    json_fields = []

    # Merge multiple devices with semicolon separator
    if all_mtd_strs:
        merged_mtd = ";".join(all_mtd_strs)
        json_fields.append('\t\t"mtd" : "{}"'.format(merged_mtd))

    if all_ubi_strs:
        merged_ubi = ";".join(all_ubi_strs)
        json_fields.append('\t\t"ubi" : "{}"'.format(merged_ubi))

    if all_nftl_strs:
        merged_nftl = ";".join(all_nftl_strs)
        json_fields.append('\t\t"nftl" : "{}"'.format(merged_nftl))

    if all_levelx_strs:
        merged_levelx = ";".join(all_levelx_strs)
        json_fields.append('\t\t"levelx" : "{}"'.format(merged_levelx))

    if all_gpt_strs:
        merged_gpt = ";".join(all_gpt_strs)
        json_fields.append('\t\t"gpt" : "{}"'.format(merged_gpt))

    # Deduplicate part_type and add to fields
    unique_part_types = list(dict.fromkeys(all_part_types))
    json_fields.append('\t\t"type": {}'.format(str(unique_part_types).replace("'", '"')))

    # Join fields with comma and newline
    part_str += ",\n".join(json_fields) + "\n"
    part_str += "\t}\n}"
    return part_str


def aic_create_two_flash_partstr_legacy(cfg, media_type):
    mtd = ""
    nftl = ""
    part_str = ""
    fal_cfg = "\n"
    part_size = 0
    part_offs = 0
    total_siz = 0

    total_siz = size_str_to_int(cfg[media_type]["size"])
    partitions = cfg[media_type]["partitions"]
    mtd = "spi{}.0:".format(cfg["image"]["info"]["media"]["device_id"])
    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)
    if media_type == "spi-nor":
        fal_cfg += "\n#ifdef FAL_PART_HAS_TABLE_CFG\n"
        fal_cfg += "#define FAL_PART_TABLE \\\n{ \\\n"
    for part in partitions:
        itemstr = ""
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
            sys.exit(1)
        part_offs += part_size
        itemstr += partitions[part]["size"]
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            itemstr += "@{}".format(partitions[part]["offset"])
            part_offs = size_str_to_int(partitions[part]["offset"])
        itemstr += "({})".format(part)
        mtd += itemstr + ","

        if media_type == "spi-nor":
            fal_cfg += "    {}FAL_PART_MAGIC_WORD, \"{}\",".format("{", part)
            fal_cfg += "FAL_USING_NOR_FLASH_DEV_NAME, "
            fal_cfg += "{},{},0{}, \\\n".format(part_offs, part_size, "}")
    # seconed flash
    total_siz = size_str_to_int(cfg[media_type]["size1"])
    partitions = cfg[media_type]["partitions1"]
    media_type1 = cfg["image"]["info"]["media1"]["type"]

    # handle mtd&fal
    mtd = mtd[0:-1]
    mtd += ";"
    mtd += "spi{}.0:".format(cfg["image"]["info"]["media1"]["device_id"])
    part_size = 0
    part_offs = 0
    for part in partitions:
        itemstr = ""
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
            sys.exit(1)
        part_offs += part_size
        itemstr += partitions[part]["size"]
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            itemstr += "@{}".format(partitions[part]["offset"])
            part_offs = size_str_to_int(partitions[part]["offset"])
        itemstr += "({})".format(part)
        mtd += itemstr + ","

        if media_type1 == "spi-nor":
            fal_cfg += "    {}FAL_PART_MAGIC_WORD, \"{}\",".format("{", part)
            fal_cfg += "FAL_USING_NOR_FLASH_DEV_NAME1, "
            fal_cfg += "{},{},0{}, \\\n".format(part_offs, part_size, "}")

        if "nftl" in partitions[part]:
            nftl_volumes = partitions[part]["nftl"]
            if len(nftl_volumes) == 0:
                print("Volume of {} is empty".format(part))
                sys.exit(1)
            nftl += "{}:".format(part)
            for vol in nftl_volumes:
                itemstr = ""
                if "size" not in nftl_volumes[vol]:
                    print("No size value for ubi volume: {}".format(vol))
                    sys.exit(1)
                itemstr += nftl_volumes[vol]["size"]
                if "offset" in nftl_volumes[vol]:
                    itemstr += "@{}".format(nftl_volumes[vol]["offset"])
                itemstr += "({})".format(vol)
                nftl += itemstr + ","
            nftl = nftl[0:-1] + ";"

    # seconed flash
    mtd = mtd[0:-1]
    part_str = "#define IMAGE_CFG_JSON_PARTS_MTD \"{}\"\n".format(mtd)
    if len(nftl) > 0:
        nftl = nftl[0:-1]
        part_str += "#define IMAGE_CFG_JSON_PARTS_NFTL \"{}\"\n".format(nftl)

    if media_type == "spi-nor":
        fal_cfg += "}\n#endif\n"
        part_str += fal_cfg

    return part_str


def aic_create_parts_string(cfg):
    # Internal helper function to process a single media_type for C string output
    def _process_single_media_string(mt, device_name, ctrl_id, part_size,
                                     part_offs, total_siz, device_idx=0):
        """Process a single media_type and return partition strings (not C defines)"""
        mtd = ""
        ubi = ""
        nftl = ""
        gpt = ""
        levelx = ""
        fal_entries = ""  # Changed from fal_cfg to collect only entries

        if _is_legacy_two_flash(cfg, device_name):
            # Legacy two-flash should be handled at upper level
            # Return None to indicate this needs special handling
            return None, None, None, None, None, None

        elif mt == "spi-nand" or mt == "spi-nor":
            total_siz = size_str_to_int(cfg[device_name]["size"])
            partitions = cfg[device_name]["partitions"]
            mtd = "spi{}.0:".format(ctrl_id)
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)

            part_size = 0
            part_offs = 0
            for part in partitions:
                itemstr = ""
                if "size" not in partitions[part]:
                    print("No size value for partition: {}".format(part))
                    sys.exit(1)
                # Update offset by adding previous partition size
                part_offs += part_size
                itemstr += partitions[part]["size"]
                part_size = size_str_to_int(partitions[part]["size"])

                if partitions[part]["size"] == "-":
                    part_size = total_siz - part_offs

                if "offset" in partitions[part]:
                    itemstr += "@{}".format(partitions[part]["offset"])
                    part_offs = size_str_to_int(partitions[part]["offset"])

                itemstr += "({})".format(part)
                mtd += itemstr + ","

                # Handle ubi volumes
                if "ubi" in partitions[part]:
                    ubi += _build_volumes_string(part, partitions[part]["ubi"], "ubi")

                # Handle nftl volumes
                if "nftl" in partitions[part]:
                    nftl += _build_volumes_string(part, partitions[part]["nftl"], "nftl")

                # Handle levelx volumes
                if "levelx" in partitions[part]:
                    levelx += _build_volumes_string(part, partitions[part]["levelx"], "levelx")

                # Generate FAL config entries for spi-nor (collect entries only)
                if mt == "spi-nor":
                    # Use device-specific flash device name if multiple devices exist
                    if device_idx > 0:
                        fal_entries += "    {}FAL_PART_MAGIC_WORD, \"{}\",".format("{", part)
                        fal_entries += "FAL_USING_NOR_FLASH_DEV_NAME{}, ".format(device_idx)
                    else:
                        fal_entries += "    {}FAL_PART_MAGIC_WORD, \"{}\",".format("{", part)
                        fal_entries += "FAL_USING_NOR_FLASH_DEV_NAME, "
                    fal_entries += "{},{},0{}, \\\n".format(part_offs, part_size, "}")

            mtd = mtd[0:-1]

            return mtd, ubi, nftl, levelx, gpt, fal_entries

        elif mt == "mmc":
            partitions = cfg[device_name]["partitions"]
            if len(partitions) == 0:
                print("Partition table is empty")
                sys.exit(1)

            for part in partitions:
                itemstr, _, _ = _build_partition_item(part, partitions[part], 0)
                gpt += itemstr + ","
            gpt = gpt[0:-1]
            return mtd, ubi, nftl, levelx, gpt, fal_entries

        else:
            print("Not supported media type: {}".format(mt))
            sys.exit(1)

    # Extract media info (support both old and new format)
    media_list = _extract_media_info(cfg)

    all_mtd_strs = []
    all_ubi_strs = []
    all_nftl_strs = []
    all_levelx_strs = []
    all_gpt_strs = []
    all_fal_entries = []  # Collect FAL entries from all devices
    spi_nor_count = 0  # Count spi-nor devices

    for media in media_list:
        device_name = media["name"]
        media_type = media["type"]
        ctrl_id = media["controller"]

        # Check if this is a legacy two-flash configuration
        if _is_legacy_two_flash(cfg, device_name):
            # Legacy config: use the old function which handles two flashes together
            defines = aic_create_two_flash_partstr_legacy(cfg, media_type)
            return defines

        mtd, ubi, nftl, levelx, gpt, fal_entries = _process_single_media_string(
            media_type, device_name, ctrl_id, 0, 0, 0, spi_nor_count)

        # Count spi-nor devices
        if media_type == "spi-nor":
            spi_nor_count += 1

        # Collect partition strings from each device
        if mtd:
            all_mtd_strs.append(mtd)
        if ubi:
            all_ubi_strs.append(ubi)
        if nftl:
            all_nftl_strs.append(nftl)
        if levelx:
            all_levelx_strs.append(levelx)
        if gpt:
            all_gpt_strs.append(gpt)
        if fal_entries:
            all_fal_entries.append(fal_entries)

    # Build final C defines by merging multiple devices
    result = ""

    # Merge MTD partitions with semicolon separator
    if all_mtd_strs:
        merged_mtd = ";".join(all_mtd_strs)
        result += "#define IMAGE_CFG_JSON_PARTS_MTD \"{}\"\n".format(merged_mtd)

    # Merge UBI partitions
    if all_ubi_strs:
        # Remove trailing semicolon from each device's ubi string before merging
        cleaned_ubi_strs = [s[:-1] if s.endswith(';') else s for s in all_ubi_strs]
        merged_ubi = ";".join(cleaned_ubi_strs)
        result += "#define IMAGE_CFG_JSON_PARTS_UBI \"{}\"\n".format(merged_ubi)

    # Merge NFTL partitions
    if all_nftl_strs:
        # Remove trailing semicolon from each device's nftl string before merging
        cleaned_nftl_strs = [s[:-1] if s.endswith(';') else s for s in all_nftl_strs]
        merged_nftl = ";".join(cleaned_nftl_strs)
        result += "#define IMAGE_CFG_JSON_PARTS_NFTL \"{}\"\n".format(merged_nftl)

    # Merge LEVELX partitions
    if all_levelx_strs:
        # Remove trailing semicolon from each device's levelx string before merging
        cleaned_levelx_strs = [s[:-1] if s.endswith(';') else s for s in all_levelx_strs]
        merged_levelx = ";".join(cleaned_levelx_strs)
        result += "#define IMAGE_CFG_JSON_PARTS_LEVELX \"{}\"\n".format(merged_levelx)

    # Merge GPT partitions
    if all_gpt_strs:
        merged_gpt = ";".join(all_gpt_strs)
        result += "#define IMAGE_CFG_JSON_PARTS_GPT \"{}\"\n".format(merged_gpt)

    # Add FAL configuration (only for spi-nor) - merge all entries into one table
    if all_fal_entries:
        result += "\n#ifdef FAL_PART_HAS_TABLE_CFG\n"
        result += "#define FAL_PART_TABLE \\\n{ \\\n"
        # Join entries without extra newlines - each entry already ends with \\n
        result += "".join(all_fal_entries)
        result += "}\n#endif\n"

    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output partition file")
    parser.add_argument("-j", "--json", type=str,
                        help="output partition json file")
    args = parser.parse_args()
    if args.config is None:
        print('Error, option --config is required.')
        sys.exit(1)

    # Parse the image configuration file specified by the --config argument
    # This loads and processes the JSON configuration file, removing comments
    # and handling formatting
    cfg = parse_image_cfg(args.config)

    # Generate partition table in JSON format based on the parsed configuration
    # This creates a structured representation of flash/mmc partitions for different media types
    parts = aic_create_parts_json(cfg)

    # Output the JSON partition table either to stdout or to a specified file
    if args.json is None:
        # If no output file is specified (--json option not used), print to console
        print(parts)
    else:
        # Write the JSON partition table to the specified output file
        with open(args.json, "w+") as f:
            f.write(parts)
    # Generate partition table in C header format based on the parsed configuration
    # This creates macro definitions for MTD/UBI/NFTL/LEVELX/GPT partitions
    parts = aic_create_parts_string(cfg)

    # Output the C header partition table either to stdout or to a specified file
    if args.outfile is None:
        # If no output file is specified (--outfile option not used), print to console
        print(parts)
    else:
        # Write the C header partition table to the specified output file
        # Include automatic generation warning and header guards to prevent multiple inclusion
        with open(args.outfile, "w+") as f:
            f.write("/* This is an auto generated file, please don't modify it. */\n\n")
            f.write("#ifndef _AIC_IMAGE_CFG_JSON_PARTITION_TABLE_H_\n")
            f.write("#define _AIC_IMAGE_CFG_JSON_PARTITION_TABLE_H_\n\n")
            f.write(parts)
            f.write("\n#endif\n")
