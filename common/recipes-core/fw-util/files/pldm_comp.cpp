#include "pldm_comp.hpp"
#include <fcntl.h>    // for open()
#include <syslog.h>   // for syslog
#include <unistd.h>   // for close()
#include <libpldm-oem/fw_update.h>

#include <cstdio>

using namespace std;

int
PldmComponent::read_pkg_header_info(int fd)
{
  int ret = 0;
  vector<uint8_t> pkgData{};
  variable_field pkgVersion{};

  pkgData.resize(sizeof(pkgHeader));

  // read Package Header Info
  if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  offset += pkgData.size();

  // read ackage Header Info - Version String
  auto pkgHeader_ptr = (pldm_package_header_information*)pkgData.data();
  auto pkgVerLen = pkgHeader_ptr->package_version_string_length;
  pkgData.resize(pkgData.size() + pkgVerLen);
  if (read(fd, pkgData.data()+sizeof(pkgHeader), pkgVerLen) != (int)pkgVerLen)
  {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  offset += pkgHeader_ptr->package_version_string_length;

  // decode Package Header Info
  ret = decode_pldm_package_header_info(pkgData.data(), pkgData.size(),
                                              &pkgHeader, &pkgVersion);
  if (ret) {
    syslog(LOG_WARNING, "%s decode failed\n", __func__);
    return FORMAT_ERR::DECODE_FAIL;
  }

  // record
  store_fw_package_hdr(*pkgHeader_ptr);
  for (int i = 0; i < PLDM_FWUP_UUID_LENGTH; ++i)
    img_uuid.push_back(pkgHeader.uuid[i]);

  return FORMAT_ERR::SUCCESS;
}

int
PldmComponent::read_device_id_record(int fd)
{
  int ret = 0, device_record_cnt;
  vector<uint8_t> pkgData{};

  pkgData.resize(1);
  // read Device Id Record count
  if (read(fd, pkgData.data(), pkgData.size()) != (int)pkgData.size()) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  offset += pkgData.size();
  device_record_cnt = pkgData[0];

  for (int i = 0; i < device_record_cnt; ++i) {
    pldm_firmware_device_id_record deviceIdRecHeader{};
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
      pkgData.data(), pkgData.size(), pkgHeader.component_bitmap_bit_length,
      &deviceIdRecHeader, &applicableComponents, &compImageSetVersionStr,
      &recordDescriptors, &fwDevicePkgData);

    if (ret) {
      syslog(LOG_WARNING, "%s decode failed (device_id_record)\n", __func__);
      return FORMAT_ERR::DECODE_FAIL;
    }
    offset += pkgData.size();

    // decode Descriptors
    for (uint8_t j = 0; j < deviceIdRecHeader.descriptor_count; ++j) {
      uint16_t descriptorType = 0;
      variable_field descriptorData{};

      ret = decode_descriptor_type_length_value(
          recordDescriptors.ptr, recordDescriptors.length,
          &descriptorType, &descriptorData);
      if (ret) {
        syslog(LOG_WARNING, "%s decode failed (descriptor)\n", __func__);
        return FORMAT_ERR::DECODE_FAIL;
      }

      // record
      store_device_id_record(deviceIdRecHeader, descriptorType, descriptorData);

      auto nextDescriptorOffset =
          sizeof(pldm_descriptor_tlv().descriptor_type) +
          sizeof(pldm_descriptor_tlv().descriptor_length) +
          descriptorData.length;
      recordDescriptors.ptr += nextDescriptorOffset;
      recordDescriptors.length -= nextDescriptorOffset;
    }
  }
  return ret;
}

int
PldmComponent::read_comp_image_info(int fd)
{
  int ret = 0;
  uint16_t comp_info_cnt;
  vector<uint8_t> pkgData{};

  // read Component Image Info count
  if (read(fd, &comp_info_cnt, sizeof(comp_info_cnt)) != (int)sizeof(comp_info_cnt)) {
    syslog(LOG_WARNING, "%s Cannot read image\n", __func__);
    return FORMAT_ERR::INVALID_SIGNATURE;
  }
  offset += 2;

  for (int i = 0; i < comp_info_cnt; ++i) {
    pldm_component_image_information compImageInfo{};
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
    offset += pkgData.size();

    // decode Component Image Info
    ret = decode_pldm_comp_image_info(pkgData.data(), pkgData.size(),
                                      &compImageInfo, &compVersion);
    if (ret) {
      syslog(LOG_WARNING, "%s decode failed (%d)\n", __func__, -ret);
      return FORMAT_ERR::DECODE_FAIL;
    }

    // record
    store_comp_img_info(compImageInfo, compVersion);
  }
  return ret;
}

int PldmComponent::pldm_update(const string& image) {

  int ret;

  syslog(LOG_CRIT, "Component %s upgrade initiated", component.c_str());

  ret = obmc_pldm_fw_update(bus, eid, (char *)image.c_str());

  if (ret)
    syslog(LOG_CRIT, "Component %s upgrade fail", component.c_str());
  else
    syslog(LOG_CRIT, "Component %s upgrade completed", component.c_str());

  return ret;
}

int PldmComponent::check_UUID() const
{
  const vector<uint8_t> UUID1 = {
    0xF0, 0x18, 0x87, 0x8C, 0xCB, 0x7D, 0x49, 0x43,
    0x98, 0x00, 0xA0, 0x2F, 0x05, 0x9A, 0xCA, 0x02};

  const vector<uint8_t> UUID2 = {
    0x12, 0x44, 0xD2, 0x64, 0x8D, 0x7D, 0x47, 0x18,
    0xA0, 0x30, 0xFC, 0x8A, 0x56, 0x58, 0x7D, 0x5A
  };

  return ((UUID1 == img_uuid)||UUID2 == img_uuid) ? 0 : -1;
}

int PldmComponent::isPldmImageValid(const string& image)
{
  int fd, ret;
  offset = 0;

  fd = open(image.c_str(), O_RDONLY);
  if (fd < 0) {
    syslog(LOG_WARNING, "%s Cannot open %s for reading\n", __func__, image.c_str());
    return FORMAT_ERR::INVALID_SIGNATURE;
  }

  // decode
  ret = read_pkg_header_info(fd);
  if (ret) {
    goto exit;
  }

  ret = read_device_id_record(fd);
  if (ret) {
    goto exit;
  }

  ret = read_comp_image_info(fd);
  if (ret) {
    goto exit;
  }

  // check UUID
  ret = check_UUID();
  if (ret) {
    syslog(LOG_WARNING, "%s UUID check failed, error code: %d.\n", __func__, -ret);
    goto exit;
  }

  // TODO : check crc-32

exit:
  close(fd);
  return ret;
}

int PldmComponent::try_pldm_update(const string& image, bool force) {

  if (isPldmImageValid(image)) {
    printf("Firmware is not valid pldm firmware. update with IPMI over PLDM.\n");
    return comp_update(image);

  } else {
    if (force) {
      return pldm_update(image);
    } else {
      if (check_header_info(img_info)) {
        printf("PLDM firmware package info not valid.\n");
        return -1;
      } else {
        return pldm_update(image);
      }
    }
  }
}