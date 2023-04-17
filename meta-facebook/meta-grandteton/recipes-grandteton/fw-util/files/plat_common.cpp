#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/vr.h>
#include <libpldm/base.h>
#include <libpldm-oem/pldm.h>
#include <syslog.h>
#include "usbdbg.h"
#include "nic_ext.h"
#include "bios.h"
#include "vr_fw.h"
#include "swb_common.hpp"
#include <openbmc/kv.h>

using namespace std;

//MEB CXL VR Component
class CxlVrComponent : public VrComponent {
    std::string name;
    int update_proc(string image, bool force);
  public:
    CxlVrComponent(const string &fru, const string &comp, const string &dev_name)
        :VrComponent(fru, comp, dev_name), name(dev_name) {}
    int fupdate(string image);
    int update(string image);
    int get_version(json& j);
};

struct fruid_map {
  uint8_t fru_id;
  string fru_name;
};

struct addr_map {
  string vr_name;
  uint8_t vr_addr;
};

struct fruid_map fru_map[] = {
  {FRU_MEB_JCN1,  "mc_cxl8"},
  {FRU_MEB_JCN2,  "mc_cxl7"},
  {FRU_MEB_JCN3,  "mc_cxl6"},
  {FRU_MEB_JCN4,  "mc_cxl5"},
  {FRU_MEB_JCN9,  "mc_cxl3"},
  {FRU_MEB_JCN10, "mc_cxl4"},
  {FRU_MEB_JCN11, "mc_cxl1"},
  {FRU_MEB_JCN12, "mc_cxl2"},
};

static int
vr_master_wr_pre(const string &fru, const string &component, bool is_fw_update) {
  struct addr_map vr_addr_map[] = {
    {"vr_p0v89a",         0xC8},
    {"vr_p0v8d_pvddq_ab", 0xB0},
    {"vr_pvddq_cd",       0xB4},
  };

  fru_status status = {0, 0};
  uint8_t txbuf[MAX_TXBUF_SIZE] = {0};
  uint8_t rxbuf[MAX_RXBUF_SIZE] = {0};
  uint8_t offset = 0;
  uint8_t rxlen = 0;
  uint8_t txlen = 0;
  size_t rlen = 0;
  size_t channel_idx = 0;
  int ret = 0;
  uint16_t vr_timeout = 60; // secs
  char key[MAX_KEY_LEN] = {0};
  char ver_temp[MAX_VALUE_LEN] = {0};

  auto fru_it = find_if(begin(fru_map), end(fru_map), [fru](auto x) {
    return x.fru_name == fru;
  });
  if (fru_it == end(fru_map))
    return -1;

  if ((pal_get_pldm_fru_status(fru_it->fru_id, JCN_0_1, &status) != 0) ||
      (status.fru_prsnt != FRU_PRSNT) || (status.fru_type != CXL_CARD)) {
    return -1;
  }

  // set FW update ongoing
  if (pal_set_fw_update_ongoing(fru_it->fru_id, vr_timeout) < 0) {
    return -1;
  }

  channel_idx = distance(begin(fru_map), fru_it);

  if (is_fw_update) {
    // If it's fw update, sleep 2 secs for sensord polling all sensors
    sleep(2);
  } else {
    // If version not in cache, asks BIC to switch MUX, otherwise get from cache
    auto vr_addr_it = find_if(begin(vr_addr_map), end(vr_addr_map), [component](auto x) {
      return x.vr_name == component;
    });
    if (vr_addr_it == end(vr_addr_map))
      return -1;
    snprintf(key, sizeof(key), "%s_vr_%02xh_crc", fru.c_str(), vr_addr_it->vr_addr);
    if (kv_get(key, ver_temp, NULL, 0)) {
      sleep(2);
    } else {
      return 0;
    }
  }

  // Switch MUX to CXL
  txbuf[0] = CXL_VR_BUS_ID; // Bus
  txbuf[1] = 0xE0; // Addr
  txbuf[2] = rxlen;
  txbuf[3] = offset;
  txlen += 4;

  uint8_t channel = 0x01;
  // Set Channel BIT due to fru
  txbuf[4] = (channel << channel_idx);
  txlen += 1;

  ret = oem_pldm_ipmi_send_recv(MEB_BIC_BUS, MEB_BIC_EID,
                               NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                               txbuf, txlen,
                               rxbuf, &rlen,
			       true);
  if ( ret < 0 ) {
    cout << "ERROR: Switch MUX to CXL failed!" << endl;
    return ret;
  }

  // Switch MUX to VR
  txbuf[0] = CXL_VR_BUS_ID; // Bus
  txbuf[1] = 0xE2; // Addr
  txbuf[2] = rxlen;
  txbuf[3] = offset;
  txbuf[4] = 0x08; // Channel to VR
  ret = oem_pldm_ipmi_send_recv(MEB_BIC_BUS, MEB_BIC_EID,
                               NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ,
                               txbuf, txlen,
			       rxbuf, &rlen,
			       true);
  if ( ret < 0 ) {
    cout << "ERROR: Switch MUX to VR failed!" << endl;
    return ret;
  }

  return ret;
}

int CxlVrComponent::update_proc(string image, bool force) {
  int ret = 0;
  string comp = this->component();
  string fru  = this->fru();

  if(vr_master_wr_pre(fru, comp, true) < 0) {
    cout << "VR Switch MUX failed" << endl;
    return -1;
  }

  if (vr_probe() < 0) {
    cout << "VR probe failed!" << endl;
    return -1;
  }
  syslog(LOG_CRIT, "Component %s upgrade initiated", comp.c_str());

  ret = vr_fw_update(name.c_str(), (char *)image.c_str(), force);
  if (ret < 0) {
    cout << "ERROR: VR Firmware update failed!" << endl;
  } else {
    syslog(LOG_CRIT, "Component %s %s completed", comp.c_str(), force? "force upgrade": "upgrade");
  }

  vr_remove();
  return ret;
}

int CxlVrComponent::update(string image) {
  int ret = 0;
  ret = update_proc(image, false);
  return ret;
}

int CxlVrComponent::fupdate(string image) {
  int ret = 0;
  ret = update_proc(image, true);
  return ret;
}

int CxlVrComponent::get_version(json& j) {
  char ver[MAX_VER_STR_LEN] = {0};
  char active_key[MAX_VER_STR_LEN] = {0};

  string fru  = this->fru();
  string comp = this->component();
  auto& dev_name = this->get_dev_name();

  snprintf(active_key, sizeof(active_key), "%s_", fru.c_str());

  try {
    j["PRETTY_COMPONENT"] = dev_name;
    if (vr_probe() || vr_master_wr_pre(fru, comp, false) ||
        vr_fw_full_version(-1, dev_name.c_str(), ver, active_key)) {
      throw "Error in getting the version of " + dev_name;
    }

    vr_remove();

    // IFX: Infineon $ver, Remaining Writes: $times
    // TI:  Texas Instruments $ver
    // MPS: MPS $ver
    string str(ver);
    string tmp_str;
    string rw_str;
    size_t start;
    auto end = str.find(',');

    if (end != string::npos) {
      start = str.rfind(' ') + 1;
      rw_str = str.substr(start, str.size() - start);
      transform(rw_str.begin(), rw_str.end(), rw_str.begin(), ::tolower);
      j["REMAINING_WRITE"] = rw_str;
    } else {
      end = str.size();
    }
    start = str.rfind(' ', end);

    tmp_str = str.substr(0, start);
    string vendor_str(tmp_str);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["VENDOR"] = tmp_str;

    start++;
    tmp_str = str.substr(start, end - start);
    transform(tmp_str.begin(), tmp_str.end(),tmp_str.begin(), ::tolower);
    j["VERSION"] = tmp_str;
    if(rw_str.empty()) {
      j["PRETTY_VERSION"] = vendor_str + " " + tmp_str;
    } else {
      j["PRETTY_VERSION"] = vendor_str + " " + tmp_str + ", Remaining Write: " + rw_str;
    }
    j["VERSION_ACTIVE"] = string(j["VERSION"]);
  } catch (string& err) {
    j["VERSION"] = "NA";
  }

  // clear fw update ongoing flag
  auto fru_it = find_if(begin(fru_map), end(fru_map), [fru](auto x) {
    return x.fru_name == fru;
  });
  if (fru_it == end(fru_map))
    return -1;
  pal_set_fw_update_ongoing(fru_it->fru_id, 0);

  return FW_STATUS_SUCCESS;
}

class fw_common_config {
  public:
    fw_common_config() {
      static NicExtComponent nic0("nic0", "nic0", "nic0_fw_ver", FRU_NIC0, 0);
      if (pal_is_artemis()) {
        static SwbVrComponent vr_pesw_vcc("cb", "pesw_vr", "VR_PESW_VCC");
        static CxlVrComponent vr_jcn1_a0v8_9  ("mc_cxl8", "vr_p0v89a",         "MC CXL8 VR_P0V89A");
        static CxlVrComponent vr_jcn1_vddq_ab ("mc_cxl8", "vr_p0v8d_pvddq_ab", "MC CXL8 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn1_vddq_cd ("mc_cxl8", "vr_pvddq_cd",       "MC CXL8 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn2_a0v8_9  ("mc_cxl7", "vr_p0v89a",         "MC CXL7 VR_P0V89A");
        static CxlVrComponent vr_jcn2_vddq_ab ("mc_cxl7", "vr_p0v8d_pvddq_ab", "MC CXL7 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn2_vddq_cd ("mc_cxl7", "vr_pvddq_cd",       "MC CXL7 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn3_a0v8_9  ("mc_cxl6", "vr_p0v89a",         "MC CXL6 VR_P0V89A");
        static CxlVrComponent vr_jcn3_vddq_ab ("mc_cxl6", "vr_p0v8d_pvddq_ab", "MC CXL6 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn3_vddq_cd ("mc_cxl6", "vr_pvddq_cd",       "MC CXL6 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn4_a0v8_9  ("mc_cxl5", "vr_p0v89a",         "MC CXL5 VR_P0V89A");
        static CxlVrComponent vr_jcn4_vddq_ab ("mc_cxl5", "vr_p0v8d_pvddq_ab", "MC CXL5 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn4_vddq_cd ("mc_cxl5", "vr_pvddq_cd",       "MC CXL5 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn9_a0v8_9  ("mc_cxl3", "vr_p0v89a",         "MC CXL3 VR_P0V89A");
        static CxlVrComponent vr_jcn9_vddq_ab ("mc_cxl3", "vr_p0v8d_pvddq_ab", "MC CXL3 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn9_vddq_cd ("mc_cxl3", "vr_pvddq_cd",       "MC CXL3 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn10_a0v8_9 ("mc_cxl4", "vr_p0v89a",         "MC CXL4 VR_P0V89A");
        static CxlVrComponent vr_jcn10_vddq_ab("mc_cxl4", "vr_p0v8d_pvddq_ab", "MC CXL4 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn10_vddq_cd("mc_cxl4", "vr_pvddq_cd",       "MC CXL4 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn11_a0v8_9 ("mc_cxl1", "vr_p0v89a",         "MC CXL1 VR_P0V89A");
        static CxlVrComponent vr_jcn11_vddq_ab("mc_cxl1", "vr_p0v8d_pvddq_ab", "MC CXL1 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn11_vddq_cd("mc_cxl1", "vr_pvddq_cd",       "MC CXL1 VR_PVDDQ_CD");
        static CxlVrComponent vr_jcn12_a0v8_9 ("mc_cxl2", "vr_p0v89a",         "MC CXL2 VR_P0V89A");
        static CxlVrComponent vr_jcn12_vddq_ab("mc_cxl2", "vr_p0v8d_pvddq_ab", "MC CXL2 VR_P0V8D_PVDDQ_AB");
        static CxlVrComponent vr_jcn12_vddq_cd("mc_cxl2", "vr_pvddq_cd",       "MC CXL2 VR_PVDDQ_CD");
      } else {
        static UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02
      }
    }
};

fw_common_config _fw_common_config;
