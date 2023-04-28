#include "fw-util.h"
#include <cstdio>
#include <cstring>
#include "server.h"
#include <openbmc/pal.h>
#include "bic_pcie_sw.h"
#include <unistd.h>
#ifdef BIC_SUPPORT
#include <facebook/bic.h>

using namespace std;

inline bool is_dev_exist(const char *dev) {
  return ( access( dev, F_OK ) != -1 );
}

int PCIESWComponent::uart_update(string image) {
#define BRCM_CMD "echo -ne \"\x66\x64\x6c\x20\x30\x20\x30\x20\x79\x0D\" > %s && sleep 2 && lsz -f -X %s < %s > %s"
  constexpr auto MAX_RETRY = 10;
  constexpr auto c2_slot1 = "/dev/serial/by-path/platform-1e6b0000.usb-usb-0:1.3.2.4:1.1-port0";
  constexpr auto c1_slot1 = "/dev/serial/by-path/platform-1e6b0000.usb-usb-0:1.1.3.2.4:1.1-port0";
  constexpr auto c1_slot3 = "/dev/serial/by-path/platform-1e6b0000.usb-usb-0:1.3.3.2.4:1.1-port0";
  uint8_t bmc_location = 0;

  // get the location
  fby3_common_get_bmc_location(&bmc_location);

  // turn on
  cerr << "Enabling USB..." << endl;
  bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_HIGH);

  // which one should be used
  auto target_pesw = (bmc_location == NIC_BMC)?c2_slot1:(slot_id == FRU_SLOT1)?c1_slot1:c1_slot3;
  int i = MAX_RETRY;

  // check the platform
  cerr << "Detecting the UART device";
  do {
    if ( is_dev_exist(target_pesw) == false ) i--;
    else break;
    sleep(1);
    cerr << ".";
  } while(i > 0);

  if ( i == 0 ) {
    cerr << "device not found" << endl;
    return FW_STATUS_FAILURE;
  } else cerr << endl;

  char cmd[1024] = {0};
  snprintf(cmd, 1024, BRCM_CMD, target_pesw, image.c_str(), target_pesw, target_pesw);

  FILE* fp = popen(cmd, "r");
  // terminate it
  if ( fp == nullptr ) throw std::runtime_error("Failed to run popen function!");

  int ret = pclose(fp);
  // print what happened if needed
  if ( ret == -1 || ret > 0 ) {
    cerr << "Err: " << std::strerror(errno) << endl;
    ret = FW_STATUS_FAILURE;
  }

  // turn off
  cerr << "Disabling USB..." << endl;
  bic_set_gpio(slot_id, RST_USB_HUB_N, GPIO_LOW);
  return ret;
}

int PCIESWComponent::update_internal(string& image, bool force) {
  int ret = 0;
  uint8_t board_type = 0xff;

  try {
    pal_set_delay_after_fw_update_ongoing();
    server.ready();
    expansion.ready();

    // if 2OU is present and it's ready, get its type
    fby3_common_get_2ou_board_type(slot_id, &board_type);

    ret = bic_update_fw(slot_id, fw_comp, (char *)image.c_str(), force);

    if ( board_type == GPV3_BRCM_BOARD) {
      cerr << "12V-cycling the server..." << endl;
      pal_set_server_power(slot_id, SERVER_12V_CYCLE);
      sleep(5);
      pal_set_server_power(slot_id, SERVER_POWER_ON);
    }

  } catch (string& err) {
    cout << "Failed Reason: " << err << endl;
    return FW_STATUS_NOT_SUPPORTED;
  }
  return ret;
}

int PCIESWComponent::update(string image) {
  return update_internal(image, FORCE_UPDATE_UNSET);
}

int PCIESWComponent::fupdate(string image) {
  return update_internal(image, FORCE_UPDATE_SET);
}

int PCIESWComponent::get_ver_str(string& s, const uint8_t alt_fw_comp, const uint8_t board_type) {
  char ver[32] = {0};
  uint8_t rbuf[4] = {0};
  int ret = 0;
  uint8_t input_comp = alt_fw_comp;

  if (board_type == CWC_MCHP_BOARD) {
    switch (alt_fw_comp) {
      case FW_2OU_PESW_CFG_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_CFG_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_CFG_VER;
        }
        break;
      case FW_2OU_PESW_FW_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_FW_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_FW_VER;
        }
        break;
      case FW_2OU_PESW_BL0_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_BL0_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_BL0_VER;
        }
        break;
      case FW_2OU_PESW_BL1_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_BL1_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_BL1_VER;
        }
        break;
      case FW_2OU_PESW_PART_MAP0_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_PART_MAP0_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_PART_MAP0_VER;
        }
        break;
      case FW_2OU_PESW_PART_MAP1_VER:
        if (fw_comp == FW_GPV3_TOP_PESW) {
          input_comp = FW_2U_TOP_PESW_PART_MAP1_VER;
        } else if (fw_comp == FW_GPV3_BOT_PESW) {
          input_comp = FW_2U_BOT_PESW_PART_MAP1_VER;
        }
        break;
    }
  }

  ret = bic_get_fw_ver(slot_id, input_comp, rbuf);
  snprintf(ver, sizeof(ver), "%02X%02X%02X%02X", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  s = string(ver);
  if ( alt_fw_comp != FW_2OU_PESW_CFG_VER && alt_fw_comp != FW_2OU_PESW_FW_VER ) {
    string tmp("(");
    if ( rbuf[0] == 0x00 ) tmp += "Inactive, ";
    else tmp += "Active, ";

    if ( rbuf[1] == 0x00 ) tmp += "Invalid)";
    else tmp += "Valid)";
    s += tmp;
  }
  return ret;
}

int PCIESWComponent::print_version() {
  map<uint8_t, string> list = {{FW_2OU_PESW_CFG_VER, "PCIE Switch Config"},
                               {FW_2OU_PESW_FW_VER,  "PCIE Switch Firmware"},
                               {FW_2OU_PESW_BL0_VER, "PCIE Bootloader0"},
                               {FW_2OU_PESW_BL1_VER, "PCIE Bootloader1"},
                               {FW_2OU_PESW_PART_MAP0_VER, "PCIE Switch Partition0"},
                               {FW_2OU_PESW_PART_MAP1_VER, "PCIE Switch Partition1"}};

  string ver("");
  string board_name = name;
  string err_msg("");
  uint8_t board_type = 0xff;

  transform(board_name.begin(), board_name.end(), board_name.begin(), ::toupper);
  try {
    server.ready();
    expansion.ready();
    // if 2OU is present and it's ready, get its type
    fby3_common_get_2ou_board_type(slot_id, &board_type);
  } catch(string& err) {
    // if we failed to get its version, we print FW_2OU_PESW_FW_VER
    printf("%s %s Version: NA (%s)\n", board_name.c_str(), list[FW_2OU_PESW_FW_VER].c_str(), err.c_str());
    return FW_STATUS_SUCCESS;   
  }

  string pesw_vendor((board_type == GPV3_BRCM_BOARD)?"BRCM":"MCHP");

  for ( auto& node:list ) {
    //BRCM supports to show CFG and FW only.
    if ( (board_type == GPV3_BRCM_BOARD) && \
         ((node.first !=  FW_2OU_PESW_FW_VER) && (node.first !=  FW_2OU_PESW_CFG_VER)) ) continue;
    try {
      //Print PESW FWs
      if ( get_ver_str(ver, node.first, board_type) < 0 ) {
        throw "Error in getting the version of " + board_name;
      }
      cout << board_name << " " << pesw_vendor << " " << node.second << " Version: " << ver << endl;
    } catch(string& err) {
      printf("%s %s %s Version: NA (%s)\n", board_name.c_str(), pesw_vendor.c_str()
                                          , node.second.c_str(), err.c_str());
    }
  }
  return FW_STATUS_SUCCESS;
}

#endif

