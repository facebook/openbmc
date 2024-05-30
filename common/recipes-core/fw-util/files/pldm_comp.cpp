#include "pldm_comp.hpp"
#include <fcntl.h>    // for open()
#include <syslog.h>   // for syslog
#include <unistd.h>   // for close()
#include <libpldm/firmware_update.h>
#include <libpldm-oem/fw_update.hpp>
#include <libpldm-oem/oem_pldm.hpp>
#include <openbmc/kv.hpp>
#include <cstdio>

using namespace std;

int PldmComponent::find_image_index(uint8_t target_id) const
{
  if (comp_info.component_id == COMPONENT_VERIFY_SKIPPED)
    return -1;

  auto comp_infos = oem_get_pkg_comp_img_info();
  auto it = std::find_if(comp_infos.begin(), comp_infos.end(),
    [target_id](const component_image_info_t& comp) {
      return comp.compImageInfo.comp_identifier == target_id;
    }
  );

  if (it != comp_infos.end())
    return it-comp_infos.begin();

  return -1;
}

int PldmComponent::get_raw_image(const string& image, string& raw_image)
{
  int ret = FORMAT_ERR::SUCCESS;
  int pldm_fd = open(image.c_str(), O_RDONLY);
  if (pldm_fd < 0) {
    cerr << "Cannot open " << image << endl;
    return FORMAT_ERR::INVALID_FILE;
  }

  raw_image = "/tmp/"+fru+"_"+component+"_pldm_raw_image.bin";
  int raw_fd = open(raw_image.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (raw_fd < 0) {
    close(pldm_fd);
    cerr << "Cannot open " << raw_image << endl;
    return FORMAT_ERR::INVALID_FILE;
  }

  auto comp_infos = oem_get_pkg_comp_img_info();
  auto index = find_image_index(comp_info.component_id);
  if (index < 0) {
    ret = FORMAT_ERR::INVALID_SIGNATURE;
    cerr << "Cannot find " << comp_info.component_id << endl;
    goto exit;
  } else {
    uint32_t size = comp_infos[index].compImageInfo.comp_size;
    uint32_t offset = comp_infos[index].compImageInfo.comp_location_offset;
    vector<uint8_t> raw_data(comp_infos[index].compImageInfo.comp_size);

    // read raw image part from PLDM image
    if (lseek(pldm_fd, offset, SEEK_SET) != offset) {
      cerr << __func__ << " lseek failed." << endl;
      ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    } else {
      if (read(pldm_fd, raw_data.data(), size) < 0) {
        cerr << __func__ << " read failed." << endl;
        ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    }
    }

    // write to temp file
    if (write(raw_fd, raw_data.data(), raw_data.size()) != (int)size) {
      cerr << __func__ << " write failed." << endl;
      ret = FORMAT_ERR::INVALID_FILE;
      goto exit;
    }
  }

exit:
  close(pldm_fd);
  close(raw_fd);
  return ret;
}

int PldmComponent::del_raw_image() const
{
  string raw_image = "/tmp/"+fru+"_"+component+"_pldm_raw_image.bin";
  return remove(raw_image.c_str());
}

int PldmComponent::pldm_update(const string& image, bool is_standard_descriptor, uint8_t specified_comp) {

  int ret;

  syslog(LOG_CRIT, "FRU %s Component %s upgrade initiated", fru.c_str(), component.c_str());

  ret = oem_pldm_fw_update(bus, eid, (char *)image.c_str(), is_standard_descriptor,
    component, wait_apply_time, specified_comp);

  if (ret)
    syslog(LOG_CRIT, "FRU %s Component %s upgrade fail", fru.c_str(), component.c_str());
  else
    syslog(LOG_CRIT, "FRU %s Component %s upgrade completed", fru.c_str(), component.c_str());

  return ret;
}

int PldmComponent::check_image(string image) {
  if (oem_parse_pldm_package(image.c_str())) {
    cerr << "PLDM header is invalid." << endl;
    return FW_STATUS_FAILURE;
  }

  auto dev_records = oem_get_pkg_device_record();
  auto comp_infos = oem_get_pkg_comp_img_info();
  signed_header_t img_info{"", 0xFF, 0xFF, 0xFF, 0xFF};

  // extract vendor-defined descriptors
  for (const auto& dev_record: dev_records) {
    for (const auto& descriptor: dev_record.recordDescriptors) {
      if (descriptor.type != PLDM_FWUP_VENDOR_DEFINED) {
        continue;
      }

      uint8_t descriptorTitleStrType = 0;
      variable_field descriptorTitleStr{};
      variable_field descriptorData{};
      variable_field data{};
      data.ptr = reinterpret_cast<const uint8_t*>(descriptor.data.data());
      data.length = descriptor.data.length();

      int rc = decode_vendor_defined_descriptor_value(data.ptr, data.length, 
                &descriptorTitleStrType, &descriptorTitleStr, &descriptorData);
      if (rc) {
        cerr << "Decoding vendor descriptor type, length and value failed. "
             << "rc=" << rc << endl;
        return FW_STATUS_FAILURE;
      }

      auto vendorDescriptorTitle = string(
        reinterpret_cast<const char*>(descriptorTitleStr.ptr), 
        descriptorTitleStr.length);
      
      auto vendorDescriptorData = string(
        reinterpret_cast<const char*>(descriptorData.ptr), 
        descriptorData.length);

      if (vendorDescriptorTitle == "Platform") {
        img_info.project_name = vendorDescriptorData;
      } else if (vendorDescriptorTitle == "BoardID") {
        auto it = signed_info_map.board_id.find(vendorDescriptorData);
        if (it != signed_info_map.board_id.end()) {
            img_info.board_id = it->second;
        }
      } else if (vendorDescriptorTitle == "Stage") {
        auto it = signed_info_map.stage.find(vendorDescriptorData);
        if (it != signed_info_map.stage.end()) {
            img_info.stage_id = it->second;
        }
      }
    }
  }

  // multiple component images should have the same vendor, so use the first 
  // one as default to get the vendor ID if COMPONENT_VERIFY_SKIPPED
  auto& comp_image_info = comp_infos.front();
  if (comp_info.component_id != COMPONENT_VERIFY_SKIPPED) {
    for (const auto& comp: comp_infos) {
      if (comp.compImageInfo.comp_identifier == comp_info.component_id) {
        img_info.component_id = comp.compImageInfo.comp_identifier;
        comp_image_info = comp;
        break;
      }
    }
  }

  auto pos = comp_image_info.compVersion.find(" ");
  if (pos != string::npos) {
    auto vendor = comp_image_info.compVersion.substr(0, pos);
    auto it = signed_info_map.vendor_id.find(vendor);
    if (it != signed_info_map.vendor_id.end()) {
      img_info.vendor_id = it->second;
    }
  }

  return check_header_info(img_info);
}

int PldmComponent::update_internal(string image, bool force) {
  int ret;

  if (!force && !has_standard_descriptor) {
    ret = check_image(image);
    if (ret) {
      string error_msg;
      switch (ret) {
      case INFO_ERR::PROJECT_NAME_NOT_MATCH:
        error_msg = "Project name not match.";
        break;
      case INFO_ERR::BOARD_NOT_MATCH:
        error_msg = "Board ID not match.";
        break;
       case INFO_ERR::COMPONENT_NOT_MATCH:
        error_msg = "Component ID not match.";
        break;
       case INFO_ERR::SOURCE_NOT_MATCH:
        error_msg = "Vendor ID not match.";
        break;
      default:
        error_msg = "Unknow error, ret=" + to_string(ret) + ".";
        break;
      }
      cerr << "Package information is invalid. " << error_msg << endl;
      return FW_STATUS_FAILURE;
    }
  }

  syslog(LOG_CRIT, "FRU %s Component %s upgrade initiated", 
    fru.c_str(), component.c_str());

  ret = oem_pldm_fw_update(bus, eid, image.c_str(), has_standard_descriptor, 
                           component, wait_apply_time, component_identifier);
  if (ret) {
    syslog(LOG_CRIT, "FRU %s Component %s upgrade fail", 
      fru.c_str(), component.c_str());
    return ret;
  }

  ret = update_version_cache();
  if (ret) {
    cerr << "Failed to update version cache." << endl;
  }

  syslog(LOG_CRIT, "FRU %s Component %s upgrade completed", 
    fru.c_str(), component.c_str());
  return ret;
}

int PldmComponent::update_version_cache() {
  firmware_parameters firmwareParameters;
  string activeVersion, pendingVersion;

  try {
    if (pldm_get_firmware_parameters(bus, eid, firmwareParameters)) {
      throw runtime_error("Failed to get firmware parameters");
    }

    if (component_identifier != PLDM_COMPONENT_IDENTIFIER_DEFAULT) {
      // is a specific component
      auto it = firmwareParameters.compParameters.find(component_identifier);
      if (it != firmwareParameters.compParameters.end()) {
        activeVersion = it->second.activeComponentVersion;
        pendingVersion = it->second.pendingComponentVersion;
      }
    } else {
      activeVersion = firmwareParameters.activeCompImageSetVersion;
      pendingVersion = firmwareParameters.pendingCompImageSetVersion;
    }
    
    activeVersion = activeVersion.empty() ? INVALID_VERSION : activeVersion;
    pendingVersion = pendingVersion.empty() ? INVALID_VERSION : pendingVersion;
    kv::set(activeVerionKey, activeVersion, kv::region::temp);
    kv::set(pendingVersionKey, pendingVersion, kv::region::temp);
  } catch(const exception& e) {
    syslog(LOG_CRIT, "FRU %s failed to update %s version cache. %s", 
      fru.c_str(), component.c_str(), e.what());
    return FW_STATUS_FAILURE;
  }
  
  return FW_STATUS_SUCCESS;
}

int PldmComponent::update(string image) {
  return update_internal(image, false);
}

int PldmComponent::fupdate(string image) {
  return update_internal(image, true);
}

int PldmComponent::get_version(json& j) {
  string activeVersion, pendingVersion;
  try {
    activeVersion = kv::get(activeVerionKey, kv::region::temp);
    pendingVersion = kv::get(pendingVersionKey, kv::region::temp);
    if (activeVersion.find(INVALID_VERSION) != string::npos || 
        pendingVersion.find(INVALID_VERSION) != string::npos) {
      throw runtime_error("Version is invalid");
    }
  } catch(...) {
    if (update_version_cache()) {
      return FW_STATUS_FAILURE;
    }
    activeVersion = kv::get(activeVerionKey, kv::region::temp);
    pendingVersion = kv::get(pendingVersionKey, kv::region::temp);
  }
  j[ACTIVE_VERSION] = activeVersion;
  j[PENDING_VERSION] = pendingVersion;

  return FW_STATUS_SUCCESS;
}

int PldmComponent::print_version() {
  auto component_name = component;
  transform(component_name.begin(), component_name.end(), 
            component_name.begin(), ::toupper);
  try {
    json j;
    if (get_version(j)) {
      throw runtime_error("Error in getting the version of " + component_name);
    }
    auto activeVersion = j[ACTIVE_VERSION].get<string>();
    auto pendingVersion = j[PENDING_VERSION].get<string>();
    transform(activeVersion.begin(), activeVersion.end(), 
              activeVersion.begin(), ::toupper);
    transform(pendingVersion.begin(), pendingVersion.end(), 
              pendingVersion.begin(), ::toupper);
    cout << component_name << " Version: " << activeVersion << endl;
    cout << component_name << " Version after activation: " 
                           << pendingVersion << endl;
  } catch(const exception& e) {
    cout << component_name << " Version: NA (" << e.what() << ")" << endl;
  }
  return FW_STATUS_SUCCESS;
}