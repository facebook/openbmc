#ifndef _BIC_M2_DEV_H_
#define _BIC_M2_DEV_H_
#include "fw-util.h"
#include "server.h"
#include "expansion.h"
#include <string>
#include <facebook/fby3_common.h>

#define MAX_DEVICE_NUM 12
#define MAX_NUM_SERVER_FRU 4

using namespace std;

class M2DevComponent : public Component {
  uint8_t slot_id = 0;
  uint8_t fw_comp = 0;
  string name;  
  Server server;
  ExpansionBoard expansion;

  struct PowerInfo {
    bool isPrinted;
    int ret;
    uint8_t status;
    uint8_t nvme_ready;
    uint8_t ffi;
    uint8_t major_ver;
    uint8_t minor_ver;
    uint8_t additional_ver;
    uint8_t sec_major_ver;
    uint8_t sec_minor_ver;
    bool    is_freya;
  };

  enum Dev_Main_Slot {
    ON_EVEN = 0,
    ON_ODD = 1
  };

  static PowerInfo statusTable[MAX_DEVICE_NUM];
  static bool isDual[MAX_NUM_SERVER_FRU];
  static bool isScaned[MAX_NUM_SERVER_FRU];
  static Dev_Main_Slot dev_main_slot;

  void scan_all_devices(uint8_t intf, M2_DEV_INFO *m2_dev_info);
  void save_info(uint8_t idx, int ret, M2_DEV_INFO *m2_dev_info);
  void print_single(uint8_t idx);
  void print_dual(uint8_t idx, M2_DEV_INFO m2_dev_info);


  private:
    int get_ver_str(string& s, const uint8_t alt_fw_comp);
  public:
    M2DevComponent(string fru, string comp, uint8_t _slot_id, string _name, uint8_t _fw_comp)
      : Component(fru, comp), slot_id(_slot_id), fw_comp(_fw_comp), name(_name), server(_slot_id, fru), expansion(_slot_id, fru, _name, _fw_comp) {}
    int update_internal(string image, bool force);
    int fupdate(string image);
    int update(string image);
    int print_version();
};

#endif
