#include <cstdio>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <syslog.h>
#include <unistd.h>
#include <openbmc/pal.h>
#include <facebook/fby35_common.h>
#include <facebook/bic.h>
#include <facebook/bic_ipmi.h>
#include "bic_fw.h"
#include <libpldm-oem/pldm.h>
#include <libpldm-oem/fw_update.hpp>

using namespace std;

#define MAX_CMD_LEN 120

image_info BicFwComponent::check_image(const string& image, bool force) {
  int ret = 0;
  uint8_t board_id = 0, board_rev = 0;
  uint8_t type = TYPE_1OU_UNKNOWN;
  image_info image_sts = {"", false, false};

  if (force == true) {
    image_sts.result = true;
  }

  switch (fw_comp) {
    case FW_SB_BIC:
    case FW_BIC_RCVY:
      switch (fby35_common_get_slot_type(slot_id)) {
        case SERVER_TYPE_HD:
          board_id = BOARD_ID_HD;
          break;
        case SERVER_TYPE_GL:
          board_id = BOARD_ID_GL;
          break;
        case SERVER_TYPE_CL:
          board_id = BOARD_ID_SB;
          break;
        case SERVER_TYPE_JI:
          board_id = BOARD_ID_JI;
          break;
        default:
          syslog(LOG_WARNING, "Failed to identify Board ID.");
      }
      ret = get_board_rev(slot_id, BOARD_ID_SB, &board_rev);
      break;
    case FW_1OU_BIC:
    case FW_1OU_BIC_RCVY:
      if (bic_get_1ou_type(slot_id, &type) == 0) {
        switch (type) {
          case TYPE_1OU_RAINBOW_FALLS:
            board_id = BOARD_ID_RF;
            break;
          case TYPE_1OU_VERNAL_FALLS_WITH_AST:
            board_id = BOARD_ID_VF;
            break;
          case TYPE_1OU_OLMSTEAD_POINT:
            bic_get_op_board_rev(slot_id, &board_rev, FEXP_BIC_INTF);
            board_id = BOARD_ID_OP;
            break;
        }
      }
      break;
    case FW_2OU_BIC:
    case FW_2OU_BIC_RCVY:
      bic_get_op_board_rev(slot_id, &board_rev, REXP_BIC_INTF);
      board_id = BOARD_ID_OP;
      break;
    case FW_3OU_BIC:
    case FW_3OU_BIC_RCVY:
      bic_get_op_board_rev(slot_id, &board_rev, EXP3_BIC_INTF);
      board_id = BOARD_ID_OP;
      break;
    case FW_4OU_BIC:
    case FW_4OU_BIC_RCVY:
      bic_get_op_board_rev(slot_id, &board_rev, EXP4_BIC_INTF);
      board_id = BOARD_ID_OP;
      break;
    case FW_BB_BIC:
      board_id = BOARD_ID_BB;
      ret = get_board_rev(slot_id, BOARD_ID_BB, &board_rev);
      break;
  }
  if (ret < 0) {
    cerr << "Failed to get board revision ID" << endl;
    return image_sts;
  }

  if (fby35_common_is_valid_img(image.c_str(), fw_comp, board_id, board_rev) == true) {
    image_sts.result = true;
    image_sts.sign = true;
  }

  return image_sts;
}

bool BicFwComponent::is_recovery() {
  switch(fw_comp) {
    case FW_BIC_RCVY:
    case FW_1OU_BIC_RCVY:
    case FW_2OU_BIC_RCVY:
    case FW_3OU_BIC_RCVY:
    case FW_4OU_BIC_RCVY:
      return true;
  }
  return false;
}

int BicFwComponent::update_internal(const string& image, bool force) {
  int ret = FW_STATUS_FAILURE;
  char cmd[MAX_CMD_LEN] = {0};

  try {
    if (!is_recovery()) {
      server.ready();
      expansion.ready();
    }
  } catch (string& err) {
    cerr << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }

  image_info image_sts = check_image(image, force);
  if (image_sts.result == false) {
    syslog(LOG_CRIT, "Update %s on %s Fail. File: %s is not a valid image",
           get_component_name(fw_comp), fru().c_str(), image.c_str());
    return FW_STATUS_FAILURE;
  }

  ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);
  if (ret != BIC_FW_UPDATE_SUCCESS) {
    switch (ret) {
      case BIC_FW_UPDATE_FAILED:
        cerr << this->alias_component() << ": update process failed" << endl;
        break;
      case BIC_FW_UPDATE_NOT_SUPP_IN_CURR_STATE:
        cerr << this->alias_component() << ": firmware update not supported in current state." << endl;
        break;
      default:
        cerr << this->alias_component() << ": unknow error (ret: " << ret << ")" << endl;
        break;
    }
    return FW_STATUS_FAILURE;
  }

  if (is_recovery()) {
    cout << "Performing 12V-cycle to complete the BIC recovery" << endl;
    ret = pal_set_server_power(slot_id, SERVER_12V_CYCLE);
    if (ret < 0) {
      printf("Failed to power cycle server\n");
      return ret;
    }
    printf("get new SDR cache from BIC \n");
    memset(cmd, 0, sizeof(cmd));
    snprintf(cmd, MAX_CMD_LEN, "/usr/local/bin/bic-cached -s slot%d; /usr/bin/kv set slot%d_sdr_thresh_update 1", slot_id, slot_id);   //retrieve SDR data after BIC FW update
    ret = system(cmd);
  }

  return ret;
}

int BicFwComponent::update(const string image) {
  return update_internal(image, false);
}

int BicFwComponent::fupdate(const string image) {
  return update_internal(image, true);
}

int BicFwComponent::get_ver_str(string& s) {
  int ret = 0;
  uint8_t rbuf[32] = {0};

  ret = bic_get_fw_ver(slot_id, fw_comp, rbuf);
  if (!ret) {
    stringstream ver;
    size_t len = strlen((char *)rbuf);
    if (len >= 4) {         // new version format
      ver << "oby35-" << string((char *)(rbuf + 4)) << "-v" << hex << setfill('0')
          << setw(2) << (int)rbuf[0] << setw(2) << (int)rbuf[1] << "."
          << setw(2) << (int)rbuf[2] << "." << setw(2) << (int)rbuf[3];
    } else if (len == 2) {  // old version format
      ver << "v" << hex << (int)rbuf[0] << "." << setfill('0') << setw(2) << (int)rbuf[1];
    } else {
      ver << "Format not supported";
    }
    s = ver.str();
  }

  return ret;
}

int BicFwComponent::print_version() {
  string ver("");
  string board_name = board;

  if (is_recovery()) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of " + board_name;
    }
    cout << board_name << " Bridge-IC Version: " << ver << endl;
  } catch (string& err) {
    cout << board_name + " Bridge-IC Version: NA (" + err + ")" << endl;
  }

  return FW_STATUS_SUCCESS;
}

int BicFwComponent::get_version(json& j) {
  string ver("");
  string board_name = board;

  if (is_recovery()) {
    return FW_STATUS_NOT_SUPPORTED;
  }

  try {
    server.ready();
    expansion.ready();
    if (get_ver_str(ver) < 0) {
      throw "Error in getting the version of " + board_name;
    }
    j["VERSION"] = ver;
  } catch (string& err) {
    if (err.find("empty") != string::npos) {
      j["VERSION"] = "not_present";
    } else {
      j["VERSION"] = "error_returned";
    }
  }
  return FW_STATUS_SUCCESS;
}

int PldmBicFwComponent::is_pldm_info_valid()
{
  signed_header_t img_info{};
  auto dev_records = oem_get_pkg_device_record();
  auto comp_infos = oem_get_pkg_comp_img_info();

  // right now, device record should be only one
  for(auto&descriper:dev_records[0].recordDescriptors) {
    if (descriper.type == PLDM_FWUP_VENDOR_DEFINED) {
      string type = descriper.data.c_str()+2;
      string data = descriper.data.c_str()+2+descriper.data.at(1);
      type.resize(descriper.data.at(1));
      data.resize(descriper.data.size()-descriper.data.at(1)-2);
      if (type == "Platform") {
        img_info.project_name = data;
        cout << "PLDM fw package info, Platform: " << data << endl;
      } else if (type == "BoardID") {
        auto& map = pldm_signed_info::board_map;
        img_info.board_id = (map.find(data)!=map.end()) ? map.at(data):0xFF;
        cout << "PLDM fw package info, board_id: " << data << "(" << (int)img_info.board_id << ")" << endl;
      } else if (type == "Stage") {
        auto& map = pldm_signed_info::stage_map;
        img_info.stage_id = (map.find(data)!=map.end()) ? map.at(data):0xFF;
        cout << "PLDM fw package info, Stage: " << data << "(" << (int)img_info.stage_id << ")" << endl;
      }
    }
  }

  if (comp_info.component_id != COMPONENT_VERIFY_SKIPPED) {
    for(auto&comp:comp_infos) {
      if (comp_info.component_id == comp.compImageInfo.comp_identifier) {
        string vendor;
        stringstream input_stringstream(comp.compVersion);
        img_info.component_id = comp.compImageInfo.comp_identifier;
        cout << "PLDM fw package info, component_id: " << (int)img_info.component_id << endl;
        if (getline(input_stringstream, vendor, ' ')) {
          auto& map = pldm_signed_info::vendor_map;
          img_info.vendor_id = (map.find(vendor)!=map.end()) ? map.at(vendor):0xFF;
          cout << "PLDM fw package info, vendor: " << vendor << "(" << (int)img_info.vendor_id << ")" << endl;
        }
      }
    }
  }

  return check_header_info(img_info);
}

int PldmBicFwComponent::try_pldm_update(const string& image, bool force, uint8_t specified_comp)
{
  int isValidImage = oem_parse_pldm_package(image.c_str());

  if (isValidImage == 0) {
    cout << "PLDM image detected." << endl;
  } else {
    cout << "PLDM header is invalid." << endl;
    return -1;
  }

  if (force) {
    return pldm_update(image, false, specified_comp);
  } else {
    if (is_pldm_info_valid() == 0) {
      cout << "PLDM Package info check passed." << endl;
      return pldm_update(image, false, specified_comp);
    } else {
      cout << "PLDM Package info is invalid." << endl;
      return -1;
    }
  }

}

int PldmBicFwComponent::update(string image)
{
  return try_pldm_update(image, false);
}

int PldmBicFwComponent::fupdate(string image)
{
  return try_pldm_update(image, true);
}
