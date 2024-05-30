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
#include <map>
#include <vector>
#include <libpldm/firmware_update.h>
#include <libpldm/utils.h>

#define PLDM_CMD_QUERY_DOWNSTREAM_DEVICE_IDENTIFIERS 0x04
#define PLDM_QUERY_DOWNSTREAM_DEVICE_IDENTIFIERS_WAIT_TIME_US 100000

enum class TRANSFER_OPERATION_FLAG : uint8_t
{
  GET_NEXT_PART,
  GET_FIRST_PART,
};

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

struct query_downstream_device_identifier_req {
  uint32_t datatransferhandle;
  TRANSFER_OPERATION_FLAG transferoperationflag;
};

struct query_downstream_device_identifier_resp {
  uint8_t complete_code;
  uint32_t nextdatatransferhandle;
  uint8_t transferflag;
  uint32_t downstreamdevicelength;
  uint16_t downstreamdevicenum;
  uint16_t downstreamdeviceidx;
  uint8_t downstreamdevicedescriptorcount;
} __attribute__((packed));

struct component_parameter {
  std::string activeComponentVersion;
  std::string pendingComponentVersion;
  pldm_component_parameter_entry componentData;  
};

struct firmware_parameters {
  bitfield32_t capabilitiesDuringUpdate;
  uint16_t compCount;
  std::string activeCompImageSetVersion;
  std::string pendingCompImageSetVersion;
  std::map<uint16_t /*component ID*/, component_parameter> compParameters;
};

int oem_parse_pldm_package (const char *path);
int oem_pldm_fw_update (uint8_t bus, uint8_t eid, const char *path, bool is_standard_descriptor, 
    std::string component, int wait_apply_time = 0, uint8_t specified_comp = 0xFF);
int pldm_get_firmware_parameters(uint8_t bus, uint8_t eid, 
    firmware_parameters& firmwareParameters);

const std::vector<firmware_device_id_record_t>& oem_get_pkg_device_record();
const std::vector<component_image_info_t>& oem_get_pkg_comp_img_info();
