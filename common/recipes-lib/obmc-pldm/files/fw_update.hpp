/*
 * Copyright 2020-present Meta Platforms, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#pragma once
#include <vector>
#include <libpldm/firmware_update.h>

struct device_id_record_descriptor {
  uint16_t type;
  std::string data;
};

struct firmware_device_id_record_t {
  pldm_firmware_device_id_record deviceIdRecHeader;
  std::string applicableComponents;
  std::string compImageSetVersionStr;
  std::string fwDevicePkgData;
  std::vector<struct device_id_record_descriptor> recordDescriptors;
};

struct component_image_info_t {
  pldm_component_image_information compImageInfo;
  std::string compVersion;
};

int oem_parse_pldm_package (const char *path);
int oem_pldm_fw_update (uint8_t bus, uint8_t eid, const char *path, uint8_t specified_comp = 0xFF);

const std::vector<firmware_device_id_record_t>& oem_get_pkg_device_record();
const std::vector<component_image_info_t>& oem_get_pkg_comp_img_info();