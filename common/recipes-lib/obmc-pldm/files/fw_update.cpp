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