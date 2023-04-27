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

#include <iostream>
#include <iomanip>
#include <syslog.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include "pldm.h"
#include "oem_pldm.hpp"
#include "fw_update.hpp"

#define MAX_TRANSFER_SIZE 1024

using namespace std;

enum FORMAT_ERR {
  SUCCESS = 0,
  INVALID_SIGNATURE,
  INVALID_FILE,
  DECODE_FAIL
};

enum FW_DEVICE_STATUS {
  IDLE = 0x00,
  LEARN_COMPONENTS,
  READY_XFER,
  DOWNLOAD,
  VERIFY,
  APPLY,
  ACTIVATE
};

enum TRANSFER_TAG {
  Start = 0x1,
  Middle = 0x2,
  End = 0x4,
  StartAndEnd = 0x5
};

static uint32_t crc_table[] = {
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
  0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
  0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
  0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
  0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
  0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
  0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
  0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
  0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
  0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
  0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
  0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
  0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
  0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
  0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
  0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
  0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
  0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
  0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
  0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
  0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
  0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

const int maxTransferSize = 1024;
// It will re-init when function call oem_parse_pldm_package()
pldm_package_header_information pkg_header{};
vector<firmware_device_id_record_t> pkg_devices{};
vector<component_image_info_t> pkg_comps{};

static string
variable_field_to_str(const variable_field& field)
{
  string ret{};
  if (field.length > 0) {
    ret = string((const char*)field.ptr);
    ret.resize(field.length);
  }
  return ret;
}

static int
read_pkg_header_info(int fd)
{
  int ret = 0;
  vector<uint8_t> pkgData{};
  variable_field pkgVersion{};

  pkgData.resize(sizeof(pkg_header));

  // read Package Header Info
  if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  // read ackage Header Info - Version String
  auto pkg_header_ptr = (pldm_package_header_information*)pkgData.data();
  auto pkgVerLen = pkg_header_ptr->package_version_string_length;
  pkgData.resize(pkgData.size() + pkgVerLen);
  if (read(fd, pkgData.data()+sizeof(pkg_header), pkgVerLen) != (int)pkgVerLen)
  {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  // decode Package Header Info
  ret = decode_pldm_package_header_info(pkgData.data(), pkgData.size(),
                                              &pkg_header, &pkgVersion);
  if (ret) {
    syslog(LOG_WARNING, "%s decode failed\n", __func__);
    return FORMAT_ERR::DECODE_FAIL;
  }

  return FORMAT_ERR::SUCCESS;
}

static int
read_device_id_record(int fd)
{
  int ret = 0, device_record_cnt;
  vector<uint8_t> pkgData{};

  pkgData.resize(1);
  // read Device Id Record count
  if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  device_record_cnt = pkgData[0];

  for (int i = 0; i < device_record_cnt; ++i) {
    firmware_device_id_record_t dev_record{};
    auto& deviceIdRecHeader = dev_record.deviceIdRecHeader;
    variable_field applicableComponents{};
    variable_field compImageSetVersionStr{};
    variable_field recordDescriptors{};
    variable_field fwDevicePkgData{};

    // read Device Id Record header
    pkgData.resize(sizeof(deviceIdRecHeader));
    if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
      syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
      return FORMAT_ERR::INVALID_SIGNATURE;
    }

    // read Device Id Record package
    auto devIdRecPtr = (pldm_firmware_device_id_record*)pkgData.data();
    auto remainDataSize = devIdRecPtr->record_length-sizeof(deviceIdRecHeader);
    pkgData.resize(devIdRecPtr->record_length);
    if (read(fd, pkgData.data()+sizeof(deviceIdRecHeader), remainDataSize)
          != (int)remainDataSize)
    {
      syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
      return FORMAT_ERR::INVALID_SIGNATURE;
    }

    // decode Device Id Record header
    ret = decode_firmware_device_id_record(
      pkgData.data(), pkgData.size(), pkg_header.component_bitmap_bit_length,
      &deviceIdRecHeader, &applicableComponents, &compImageSetVersionStr,
      &recordDescriptors, &fwDevicePkgData);

    if (ret) {
      syslog(LOG_WARNING, "%s decode failed (device_id_record)\n", __func__);
      return FORMAT_ERR::DECODE_FAIL;
    }
    dev_record.applicableComponents = variable_field_to_str(applicableComponents);
    dev_record.compImageSetVersionStr = variable_field_to_str(compImageSetVersionStr);
    dev_record.fwDevicePkgData = variable_field_to_str(fwDevicePkgData);

    // decode Descriptors
    dev_record.recordDescriptors.clear();
    for (uint8_t j = 0; j < deviceIdRecHeader.descriptor_count; ++j) {
      device_id_record_descriptor descriptor{};
      variable_field descriptorData{};

      ret = decode_descriptor_type_length_value(
          recordDescriptors.ptr, recordDescriptors.length,
          &descriptor.type, &descriptorData);
      if (ret) {
        syslog(LOG_WARNING, "%s decode failed (descriptor)\n", __func__);
        return FORMAT_ERR::DECODE_FAIL;
      }
      descriptor.data = variable_field_to_str(descriptorData);
      dev_record.recordDescriptors.emplace_back(descriptor);

      auto nextDescriptorOffset =
          sizeof(pldm_descriptor_tlv().descriptor_type) +
          sizeof(pldm_descriptor_tlv().descriptor_length) +
          descriptorData.length;
      recordDescriptors.ptr += nextDescriptorOffset;
      recordDescriptors.length -= nextDescriptorOffset;
    }

    pkg_devices.emplace_back(dev_record);
  }
  return ret;
}

static int
read_comp_image_info(int fd)
{
  int ret = 0;
  uint16_t comp_info_cnt;
  vector<uint8_t> pkgData{};

  // read Component Image Info count
  if (read(fd, &comp_info_cnt, sizeof(comp_info_cnt)) != (int)sizeof(comp_info_cnt)) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  for (int i = 0; i < comp_info_cnt; ++i) {
    component_image_info_t comp_info{};
    auto& compImageInfo = comp_info.compImageInfo;
    variable_field compVersion{};

    // read Component Image Info
    pkgData.resize(sizeof(compImageInfo));
    if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
      syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
      return FORMAT_ERR::INVALID_SIGNATURE;
    }

    // read Component Image Info - version string
    auto compImageInfo_ptr = (pldm_component_image_information*)pkgData.data();
    auto compImageVerLen = compImageInfo_ptr->comp_version_string_length;
    pkgData.resize(pkgData.size() + compImageVerLen);
    if (read(fd, pkgData.data()+sizeof(compImageInfo), compImageVerLen)
        != (int)compImageVerLen)
    {
      syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
      return FORMAT_ERR::INVALID_SIGNATURE;
    }

    // decode Component Image Info
    ret = decode_pldm_comp_image_info(pkgData.data(), pkgData.size(),
                                      &compImageInfo, &compVersion);
    if (ret) {
      syslog(LOG_WARNING, "%s decode failed (%d)\n", __func__, -ret);
      return FORMAT_ERR::DECODE_FAIL;
    }

    // record
    comp_info.compVersion = variable_field_to_str(compVersion);
    pkg_comps.emplace_back(comp_info);
    // store_comp_img_info(compImageInfo, compVersion);
  }
  return ret;
}

static uint32_t pldm_crc32(const uint8_t* buffer, size_t length)
{
  // Compute the CRC-32 checksum of the buffer
  uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; i++) {
    crc = crc_table[(crc ^ buffer[i]) & 0xFF] ^ (crc >> 8);
  }

  // Invert the final CRC value and return it
  return crc ^ 0xFFFFFFFFu;
}

static int
read_pkg_hdr_check(int fd)
{
  uint32_t pkg_checksum, checksum;
  vector<uint8_t> pkg_hdr;
  off_t offset = lseek(fd, 0, SEEK_CUR);

  if (read(fd, &pkg_checksum, sizeof(uint32_t)) != sizeof(pkg_checksum)) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_FILE;
  }

  pkg_hdr.resize(offset);
  lseek(fd, 0, SEEK_SET);
  if (read(fd, pkg_hdr.data(), offset) != offset) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_FILE;
  }
  checksum = pldm_crc32(pkg_hdr.data(), pkg_hdr.size());

  return (pkg_checksum == checksum) ? FORMAT_ERR::SUCCESS:FORMAT_ERR::INVALID_SIGNATURE;
}

static bool
is_uuid_match(const vector<uint8_t>& uuid)
{
  vector<uint8_t> image_uuid(pkg_header.uuid, pkg_header.uuid+PLDM_FWUP_UUID_LENGTH);
  return (image_uuid == uuid);
}

static int
is_uuid_valid()
{
  static const vector<vector<uint8_t>> uuids = {
    {0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43,
     0x98, 0x00, 0xA0, 0x2F, 0x05, 0x9A, 0xCA, 0x02}, // 1.0.x
    {0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
     0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5A}, // 1.1.x
    {0x31, 0x19, 0xCE, 0x2F, 0xE8, 0x0A, 0x4A, 0x99,
     0xAF, 0x6D, 0x46, 0xF8, 0xB1, 0x21, 0xF6, 0xBF}  // 1.2.x
  };

  return (any_of(uuids.begin(), uuids.end(), is_uuid_match)) ? 0 : 1;
}

int oem_parse_pldm_package (const char *path)
{
  int ret = -1;
  pkg_header = {};
  pkg_devices = {};
  pkg_comps = {};

  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    cerr << "Cannot open " << path << endl;
    return -1;
  }

  if (read_pkg_header_info(fd)) {
    syslog(LOG_WARNING, "Package Header Info is unvalid.");
    goto exit;
  }

  if (read_device_id_record(fd)) {
    syslog(LOG_WARNING, "Device Id Record is unvalid.");
    goto exit;
  }

  if (read_comp_image_info(fd)) {
    syslog(LOG_WARNING, "Component Image Info is unvalid.");
    goto exit;
  }

  if (read_pkg_hdr_check(fd)) {
    syslog(LOG_WARNING, "Package Header Checksum is unvalid.");
    goto exit;
  }

  if (is_uuid_valid()) {
    syslog(LOG_WARNING, "UUID is unvalid.");
    goto exit;
  }

  ret = 0;
exit:
  close(fd);
  return ret;
}

static int
pldm_get_status(int fd, uint8_t eid, uint8_t& curr_stat)
{
  int ret;
  vector<uint8_t> response{};
  vector<uint8_t> request = {0x80, PLDM_FWUP, PLDM_GET_STATUS};

  ret = oem_pldm_send_recv_w_fd(fd, eid, request, response);
  if (ret == 0 && response[sizeof(pldm_msg_hdr)] == 0x00)
    curr_stat = response[sizeof(pldm_msg_hdr)+1];

  return ret;
}

static int
pldm_cancel_update(int fd, uint8_t eid)
{
  vector<uint8_t> response{};
  vector<uint8_t> request = {0x80, PLDM_FWUP, PLDM_CANCEL_UPDATE};

  return oem_pldm_send_recv_w_fd(fd, eid, request, response);
}

static int
pldm_request_update(int fd, uint8_t eid, uint8_t record_id)
{
  auto pkgData_len = pkg_devices[record_id].deviceIdRecHeader.fw_device_pkg_data_length;
  auto compVer_len = pkg_devices[record_id].deviceIdRecHeader.comp_image_set_version_string_length;
  variable_field compVer{};
  compVer.length = pkg_devices[record_id].compImageSetVersionStr.size();
  compVer.ptr = (const uint8_t*)pkg_devices[record_id].compImageSetVersionStr.c_str();

  vector<uint8_t> response{};
  vector<uint8_t> request(
    sizeof(pldm_msg_hdr) +
    sizeof(struct pldm_request_update_req) +
    compVer_len
  );
  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

  int ret = encode_request_update_req (
    0x00, // instance id
    maxTransferSize,
    pkg_comps.size(),
    PLDM_FWUP_MIN_OUTSTANDING_REQ,
    pkgData_len,
    PLDM_STR_TYPE_ASCII,
    compVer_len,
    &compVer,
    requestMsg,
    sizeof(struct pldm_request_update_req) + compVer_len
  );
  if (ret != PLDM_SUCCESS) {
    cerr << "Failed to encode request update request."
         << " rc = " << ret
         << endl;
    return -1;
  }

  return oem_pldm_send_recv_w_fd(fd, eid, request, response);
}

static uint8_t
pldm_get_tag(size_t curr, size_t end)
{
  if (curr == 0) {
    if (curr == end-1)
      return TRANSFER_TAG::StartAndEnd;
    else
      return TRANSFER_TAG::Start;
  } else if (curr == end-1) {
    return TRANSFER_TAG::End;
  } else {
    return TRANSFER_TAG::Middle;
  }
}

static int
pldm_pass_comp_table(int fd, uint8_t eid, size_t index)
{
  uint8_t transferFlag = pldm_get_tag(index, pkg_comps.size());
  auto compVer_len = pkg_comps[index].compImageInfo.comp_version_string_length;
  variable_field compVer{};
  compVer.length = pkg_comps[index].compVersion.size();
  compVer.ptr = (const uint8_t*)pkg_comps[index].compVersion.c_str();
  vector<uint8_t> response{};
  vector<uint8_t> request (
    sizeof(pldm_msg_hdr) +
    sizeof(struct pldm_pass_component_table_req) +
    compVer_len
  );
  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

  int ret = encode_pass_component_table_req(
    0x00,
    transferFlag,
    pkg_comps[index].compImageInfo.comp_classification,
    pkg_comps[index].compImageInfo.comp_identifier,
    index,
    pkg_comps[index].compImageInfo.comp_comparison_stamp,
    PLDM_STR_TYPE_ASCII,
    compVer_len,
    &compVer,
    requestMsg,
    sizeof(pldm_pass_component_table_req) + compVer_len
  );
  if (ret != PLDM_SUCCESS) {
    cerr << "Failed to encode pass component table request."
         << "\nrc = " << ret
         << endl;
    return -1;
  }

  return oem_pldm_send_recv_w_fd(fd, eid, request, response);
}

static int
pldm_update_comp(int fd, uint8_t eid, size_t index)
{
  uint32_t updateOptionFlags =
    (pkg_comps[index].compImageInfo.comp_options.value << 16) +
    pkg_comps[index].compImageInfo.requested_comp_activation_method.value;
  auto compVer_len = pkg_comps[index].compImageInfo.comp_version_string_length;
  variable_field compVer{};
  compVer.length = pkg_comps[index].compVersion.size();
  compVer.ptr = (const uint8_t*)pkg_comps[index].compVersion.c_str();

  vector<uint8_t> response{};
  vector<uint8_t> request (
    sizeof(pldm_msg_hdr) +
    sizeof(struct pldm_update_component_req) +
    compVer_len
  );
  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

  int ret = encode_update_component_req(
    0x00,
    pkg_comps[index].compImageInfo.comp_classification,
    pkg_comps[index].compImageInfo.comp_identifier,
    index,
    pkg_comps[index].compImageInfo.comp_comparison_stamp,
    pkg_comps[index].compImageInfo.comp_size,
    bitfield32_t{updateOptionFlags},
    PLDM_STR_TYPE_ASCII,
    compVer_len,
    &compVer,
    requestMsg,
    sizeof(pldm_update_component_req) + compVer_len
  );
  if (ret != PLDM_SUCCESS) {
    cerr << "Failed to encode update component request."
         << "\nrc = " << ret
         << endl;
    return -1;
  }

  return oem_pldm_send_recv_w_fd(fd, eid, request, response);
}

static int
pldm_request_firmware_data_handle(int sockfd, uint8_t eid, size_t comps_index,
                                  vector<uint8_t>& request, int file)
{
  int ret = 0;
  uint8_t completionCode = PLDM_SUCCESS;
  uint32_t offset = 0;
  uint32_t length = 0;
  vector<uint8_t> response(sizeof(pldm_msg_hdr) + sizeof(completionCode));
  auto compSize = pkg_comps[comps_index].compImageInfo.comp_size;
  auto compOffset = pkg_comps[comps_index].compImageInfo.comp_location_offset;
  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());
  auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());

  ret = decode_request_firmware_data_req(requestMsg, request.size()-sizeof(pldm_msg_hdr),
                                          &offset, &length);
  // Decode request failed
  if (ret) {
    cerr << "Decode request firmware data request failed. rc= "
         << -ret
         << endl;
    return -1;
  // PLDM_FWUP_INVALID_TRANSFER_LENGTH
  } else if (length < PLDM_FWUP_BASELINE_TRANSFER_SIZE || length > maxTransferSize) {
    ret = encode_request_firmware_data_resp(
      requestMsg->hdr.instance_id,
      PLDM_FWUP_INVALID_TRANSFER_LENGTH,
      responseMsg, sizeof(completionCode));
    if (ret) {
      cerr << "Encoding RequestFirmwareData response failed." << endl;
      return -1;
    }
  // PLDM_FWUP_DATA_OUT_OF_RANGE
  } else if (offset + length > compSize + PLDM_FWUP_BASELINE_TRANSFER_SIZE) {
    ret = encode_request_firmware_data_resp(
      requestMsg->hdr.instance_id,
      PLDM_FWUP_DATA_OUT_OF_RANGE,
      responseMsg, sizeof(completionCode));
    if (ret) {
      cerr << "Encoding RequestFirmwareData response failed." << endl;
      return -1;
    }
  // SUCCESS
  } else {
    // encode pldm response header
    response.resize(sizeof(pldm_msg_hdr) + sizeof(completionCode) + length);

    // read file to payload
    if (lseek(file, offset+compOffset, SEEK_SET) != offset+compOffset) {
      cerr << __func__ << " lseek failed." << endl;
      return -1;
    } else {
      if (read(file, response.data()+sizeof(pldm_msg_hdr)+sizeof(completionCode), length) < 0)
      {
        cerr << __func__ << " read failed." << endl;
        return -1;
      }
    }

    responseMsg = reinterpret_cast<pldm_msg*>(response.data());
    ret = encode_request_firmware_data_resp(
      requestMsg->hdr.instance_id, completionCode,
      responseMsg, sizeof(completionCode));
    if (ret) {
      cerr << "Encoding RequestFirmwareData response failed." << endl;
      return -1;
    }

    cout << "\rDownload "
         << "offset : 0x"  << setfill('0') << setw(8) << hex << offset
         << "/0x" << setfill('0') << setw(8) << hex << compSize
         << ", size : 0x"  << setfill('0') << setw(8) << hex << length
         << flush;
  }

  return oem_pldm_send(sockfd, eid, response);
}

static int
pldm_transfer_complete_handle(int sockfd, uint8_t eid, vector<uint8_t>& request, uint8_t& status)
{
  int ret = 0;
  uint8_t completionCode = PLDM_SUCCESS;
  uint8_t transferResult = 0;

  vector<uint8_t> response(sizeof(pldm_msg_hdr) + sizeof(completionCode));
  auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());
  auto requestMsg  = reinterpret_cast<pldm_msg*>(request.data());
  ret = decode_transfer_complete_req(requestMsg, request.size()-sizeof(pldm_msg_hdr),
                                    &transferResult);
  // Decode request failed
  if (ret) {
    cerr << "Decode request transfer complete failed." << endl;
    return -1;
  }
  // Transfer result is failed
  else if (transferResult != PLDM_FWUP_TRANSFER_SUCCESS) {
    cerr << "Transfer of the component failed. result="
         << int(transferResult)
         << endl;
  }
  // SUCCESS
  else {
    status = FW_DEVICE_STATUS::VERIFY;
    cout << "\nTransferComplete." << endl;
  }

  ret = encode_transfer_complete_resp(
    requestMsg->hdr.instance_id, completionCode,
    responseMsg, sizeof(completionCode)
  );
  if (ret) {
    cerr << "Encoding TransferComplete response failed." << endl;
    return -1;
  }

  return oem_pldm_send(sockfd, eid, response);
}

static int
pldm_verify_complete_handle(int sockfd, uint8_t eid, vector<uint8_t>& request, uint8_t& status)
{
  int ret = 0;
  uint8_t completionCode = PLDM_SUCCESS;
  uint8_t verifyResult = 0;

  vector<uint8_t> response(sizeof(pldm_msg_hdr) + sizeof(completionCode));
  auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());
  auto requestMsg  = reinterpret_cast<pldm_msg*>(request.data());
  ret = decode_verify_complete_req(requestMsg, request.size()-sizeof(pldm_msg_hdr),
                                    &verifyResult);
  // Decode request failed
  if (ret) {
    cerr << "Decoding verify complete request failed." << endl;
    return -1;
  }
  // Verify result is failed
  else if (verifyResult != PLDM_FWUP_VERIFY_SUCCESS) {
    cerr << "Component verification failed, result="
         << int(verifyResult)
         << endl;
  }
  // SUCCESS
  else {
    status = FW_DEVICE_STATUS::APPLY;
    cout << "VerifyComplete." << endl;
  }

  ret = encode_verify_complete_resp(
    requestMsg->hdr.instance_id, completionCode,
    responseMsg, sizeof(completionCode)
  );
  if (ret) {
    cerr << "Encoding VerifyComplete response failed." << endl;
    return -1;
  }

  return oem_pldm_send(sockfd, eid, response);
}

static int
pldm_apply_complete_handle(int sockfd, uint8_t eid, vector<uint8_t>& request, uint8_t& status)
{
  int ret = 0;
  uint8_t completionCode = PLDM_SUCCESS;
  uint8_t applyResult = 0;
  bitfield16_t compActivationModification{};

  vector<uint8_t> response(sizeof(pldm_msg_hdr) + sizeof(completionCode));
  auto responseMsg = reinterpret_cast<pldm_msg*>(response.data());
  auto requestMsg  = reinterpret_cast<pldm_msg*>(request.data());
  ret = decode_apply_complete_req(requestMsg, request.size()-sizeof(pldm_msg_hdr),
                                    &applyResult, &compActivationModification);
  // Decode request failed
  if (ret) {
    cerr << "Decoding apply complete request failed." << endl;
    return -1;
  }
  // Apply result is failed
  else if (applyResult != PLDM_FWUP_APPLY_SUCCESS &&
           applyResult != PLDM_FWUP_APPLY_SUCCESS_WITH_ACTIVATION_METHOD) {
    cerr << "Component apply failed, result="
         << int(applyResult)
         << endl;
  }
  // SUCCESS
  else {
    status = FW_DEVICE_STATUS::READY_XFER;
    cout << "ApplyComplete." << endl;
  }

  ret = encode_apply_complete_resp(
    requestMsg->hdr.instance_id, completionCode,
    responseMsg, sizeof(completionCode)
  );
  if (ret) {
    cerr << "Encoding ApplyComplete response failed." << endl;
    return -1;
  }

  return oem_pldm_send(sockfd, eid, response);
}

static int
pldm_do_download(int sockfd, uint8_t eid, size_t index, uint8_t& curr_stat, int file)
{
  int ret = 0;
  vector<uint8_t> request{};
  if (oem_pldm_recv(sockfd, eid, request) != PLDM_SUCCESS) {
    curr_stat = FW_DEVICE_STATUS::IDLE;
    cerr << "Recv failed in download phase." << endl;
    return -1;
  }

  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());
  switch (requestMsg->hdr.command) {
    case PLDM_REQUEST_FIRMWARE_DATA:
      ret = pldm_request_firmware_data_handle(sockfd, eid, index, request, file);
      break;
    case PLDM_TRANSFER_COMPLETE:
      ret = pldm_transfer_complete_handle(sockfd, eid, request, curr_stat);
      break;
    case PLDM_VERIFY_COMPLETE:
      ret = pldm_verify_complete_handle(sockfd, eid, request, curr_stat);
      break;
    case PLDM_APPLY_COMPLETE:
      ret = pldm_apply_complete_handle(sockfd, eid, request, curr_stat);
      break;
    default:
      cerr << "Unexpected pldm command: 0x"
           << setfill('0') << setw(2) << right << hex
           << (int)requestMsg->hdr.command
           << endl;
      ret = -1;
      break;
  }

  // The things with the basic pldm connection are going wrong.
  if (ret == -1) {
    curr_stat = FW_DEVICE_STATUS::IDLE;
  }

  return ret;
}

int
pldm_activate_firmware(int sockfd, uint8_t eid)
{
  vector<uint8_t> response{};
  vector<uint8_t> request (
    sizeof(pldm_msg_hdr) +
    sizeof(struct pldm_activate_firmware_req)
  );
  auto requestMsg = reinterpret_cast<pldm_msg*>(request.data());

  int ret = encode_activate_firmware_req (
    0x00,
    PLDM_NOT_ACTIVATE_SELF_CONTAINED_COMPONENTS,
    requestMsg,
    sizeof(pldm_activate_firmware_req)
  );
  if (ret != PLDM_SUCCESS) {
    cerr << "Failed to encode request activate firmware. "
         << "rc = " << ret
         << endl;
    return -1;
  }

  cout << "ActivateFirmwareComplete." << endl;
  return oem_pldm_send_recv_w_fd(sockfd, eid, request, response);
}

int oem_pldm_fw_update(uint8_t bus, uint8_t eid, const char* path, uint8_t specified_comp)
{
  int ret, sockfd, file;
  uint8_t status = 0;

  sockfd = oem_pldm_init_fwupdate_fd(bus);
  if (sockfd < 0) {
    cerr << "Failed to connect pldm daemon." << endl;
    return -1;
  }

  ret = oem_parse_pldm_package(path);
  if (ret < 0) {
    cerr << "Failed to parse pldm package header." << endl;
    close(sockfd);
    return -1;
  } else {
    file = open(path, O_RDONLY);
    if (file < 0) {
      cerr << "Cannot open " << path << endl;
      close(sockfd);
      return -1;
    }
  }

  // try reset firmware device status
  ret = pldm_get_status(sockfd, eid, status);
  if (ret < 0) {
    cerr << "Failed to get status." << endl;
    goto exit;
  } else if (status != FW_DEVICE_STATUS::IDLE &&
              status != FW_DEVICE_STATUS::APPLY) {
    ret = pldm_cancel_update(sockfd, eid);
    if (ret < 0) {
      cerr << "Failed to reset pldm update status." << endl;
      goto exit;
    }
  }

  // It means that only one comp image UA wanna use
  // when specified_comp not equals to 0xFF. (For GT pex update)
  if (specified_comp != 0xFF) {
    auto it = find_if(pkg_comps.begin(), pkg_comps.end(),
      [specified_comp](const component_image_info_t& comp){
        return (comp.compImageInfo.comp_identifier == specified_comp);
      }
    );
    if (it == pkg_comps.end()) {
      cerr << "Failed to find corresponding component image." << endl;
      goto exit;
    } else {
      auto comp = *it;
      pkg_comps.clear();
      pkg_comps.emplace_back(comp);

    }
  }

  // send RequestUpdate (Next State: LEARN COMPONENTS)
  ret = pldm_request_update(sockfd, eid, 0); // only support 1 device record for now
  if (ret < 0) {
    cerr << "Failed to send RequestUpdate." << endl;
    goto exit;
  }
  cout << "RequestUpdate Success." << endl;
  status = FW_DEVICE_STATUS::LEARN_COMPONENTS;

  // send PassComponentTable (Next State: READY XFER)
  for (size_t i = 0; i < pkg_comps.size(); ++i) {
    ret = pldm_pass_comp_table(sockfd, eid, i);
    if (ret < 0) {
      cerr << "Failed to send PassComponentTable." << endl;
      goto exit;
    }
  }
  cout << "PassComponentTable Success." << endl;
  status = FW_DEVICE_STATUS::READY_XFER;

  // send UpdateComponent (Next State: DOWNLOAD)
  for (size_t i = 0; i < pkg_comps.size(); ++i) {
    ret = pldm_update_comp(sockfd, eid, i);
    if (ret < 0) {
      cerr << "Failed to send UpdateComponent." << endl;
      goto exit;
    } else {
      cout << "UpdateComponent Success." << endl;
      status = FW_DEVICE_STATUS::DOWNLOAD;
      while (true) {
        ret = pldm_do_download(sockfd, eid, i, status, file);
        if (ret < 0) {
          cerr << "Failed at download state ." << endl;
          goto exit;
        } else if (status == FW_DEVICE_STATUS::READY_XFER) {
          break;
        }
      }
    }
  }

exit:
  if (ret == 0 && status == FW_DEVICE_STATUS::READY_XFER)
    ret = pldm_activate_firmware(sockfd, eid);

  if (ret < 0)
    pldm_cancel_update(sockfd, eid);

  close(sockfd);
  close(file);
  return ret;
}

const vector<firmware_device_id_record_t>& oem_get_pkg_device_record()
{
  return pkg_devices;
}

const vector<component_image_info_t>& oem_get_pkg_comp_img_info()
{
  return pkg_comps;
}