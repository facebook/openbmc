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
#include <openbmc/kv.h>

using namespace std;

//SWB VR Component
class SwbVrComponent : public VrComponent {
    std::string name;
    int update_proc(string image, bool force);
  public:
    SwbVrComponent(const string &fru, const string &comp, const string &dev_name)
        :VrComponent(fru, comp, dev_name), name(dev_name) {}
    int fupdate(string image);
    int update(string image);

};

static
int set_swb_snr_polling (uint8_t status) {
  uint8_t tbuf[255] = {0};
  uint8_t rbuf[255] = {0};
  uint8_t tlen=0;
  size_t rlen = 0;
  int rc;

  tbuf[tlen++] = status;

  rc = oem_pldm_ipmi_send_recv(SWB_BUS_ID, SWB_BIC_EID,
                               NETFN_OEM_1S_REQ,
                               CMD_OEM_1S_DISABLE_SEN_MON,
                               tbuf, tlen,
                               rbuf, &rlen, true);
  printf("%s rc=%d", __func__, rc);
  return rc;
}


int SwbVrComponent::update_proc(string image, bool force) {
  int ret;
  string comp = this->component();

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


int SwbVrComponent::update(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 0);

  if(set_swb_snr_polling(0x01))
    printf("set snr polling start fail\n");

  return ret;
}

int SwbVrComponent::fupdate(string image) {
  int ret;

  ret = set_swb_snr_polling(0x00);
  if (ret)
   return ret;

  ret = update_proc(image, 1);

  if(set_swb_snr_polling(0x01))
    printf("set snr polling start fail\n");

  return ret;
}

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
  {FRU_MEB_JCN1,  "meb_jcn1"},
  {FRU_MEB_JCN2,  "meb_jcn2"},
  {FRU_MEB_JCN3,  "meb_jcn3"},
  {FRU_MEB_JCN4,  "meb_jcn4"},
  {FRU_MEB_JCN9,  "meb_jcn9"},
  {FRU_MEB_JCN10, "meb_jcn10"},
  {FRU_MEB_JCN11, "meb_jcn11"},
  {FRU_MEB_JCN12, "meb_jcn12"},
};

static int
vr_master_wr_pre(const string &fru, const string &component, bool is_fw_update) {
  struct addr_map vr_addr_map[] = {
    {"vr_a0v8_9",  0xC8},
    {"vr_vddq_ab", 0xB0},
    {"vr_vddq_cd", 0xB4},
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
        static SwbVrComponent vr_pesw_vcc("acb", "pesw_vr", "VR_PESW_VCC");
        static CxlVrComponent vr_jcn1_a0v8_9 ("meb_jcn1", "vr_a0v8_9",  "MEB JCN1 VR_A0V8_9");
        static CxlVrComponent vr_jcn1_vddq_ab("meb_jcn1", "vr_vddq_ab", "MEB JCN1 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn1_vddq_cd("meb_jcn1", "vr_vddq_cd", "MEB JCN1 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn2_a0v8_9 ("meb_jcn2", "vr_a0v8_9",  "MEB JCN2 VR_A0V8_9");
        static CxlVrComponent vr_jcn2_vddq_ab("meb_jcn2", "vr_vddq_ab", "MEB JCN2 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn2_vddq_cd("meb_jcn2", "vr_vddq_cd", "MEB JCN2 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn3_a0v8_9 ("meb_jcn3", "vr_a0v8_9",  "MEB JCN3 VR_A0V8_9");
        static CxlVrComponent vr_jcn3_vddq_ab("meb_jcn3", "vr_vddq_ab", "MEB JCN3 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn3_vddq_cd("meb_jcn3", "vr_vddq_cd", "MEB JCN3 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn4_a0v8_9 ("meb_jcn4", "vr_a0v8_9",  "MEB JCN4 VR_A0V8_9");
        static CxlVrComponent vr_jcn4_vddq_ab("meb_jcn4", "vr_vddq_ab", "MEB JCN4 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn4_vddq_cd("meb_jcn4", "vr_vddq_cd", "MEB JCN4 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn9_a0v8_9 ("meb_jcn9", "vr_a0v8_9",  "MEB JCN9 VR_A0V8_9");
        static CxlVrComponent vr_jcn9_vddq_ab("meb_jcn9", "vr_vddq_ab", "MEB JCN9 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn9_vddq_cd("meb_jcn9", "vr_vddq_cd", "MEB JCN9 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn10_a0v8_9("meb_jcn10", "vr_a0v8_9", "MEB JCN10 VR_A0V8_9");
        static CxlVrComponent vr_jcn10_vddq_ab("meb_jcn10", "vr_vddq_ab", "MEB JCN10 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn10_vddq_cd ("meb_jcn10", "vr_vddq_cd", "MEB CN10 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn11_a0v8_9 ("meb_jcn11", "vr_a0v8_9",  "MEB JCN11 VR_A0V8_9");
        static CxlVrComponent vr_jcn11_vddq_ab("meb_jcn11", "vr_vddq_ab", "MEB JCN11 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn11_vddq_cd ("meb_jcn11", "vr_vddq_cd", "MEB JCN11 VR_VDDQ_CD");
        static CxlVrComponent vr_jcn12_a0v8_9 ("meb_jcn12", "vr_a0v8_9",  "MEB JCN12 VR_A0V8_9");
        static CxlVrComponent vr_jcn12_vddq_ab("meb_jcn12", "vr_vddq_ab", "MEB JCN12 VR_VDDQ_AB");
        static CxlVrComponent vr_jcn12_vddq_cd ("meb_jcn12", "vr_vddq_cd", "MEB JCN12 VR_VDDQ_CD");
      } else {
        static UsbDbgComponent usbdbg("ocpdbg", "mcu", "F0T", 14, 0x60, false);
        static UsbDbgBlComponent usbdbgbl("ocpdbg", "mcubl", 14, 0x60, 0x02);  // target ID of bootloader = 0x02
        static SwbVrComponent vr_pex0_vcc("swb", "pex01_vcc", "VR_PEX01_VCC");
        static SwbVrComponent vr_pex1_vcc("swb", "pex23_vcc", "VR_PEX23_VCC");
      }
    }
};

fw_common_config _fw_common_config;
