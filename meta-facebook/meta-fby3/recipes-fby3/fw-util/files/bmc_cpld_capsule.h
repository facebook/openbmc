#include <string>
#include "fw-util.h"

using namespace std;

#define MAX_FILE_PATH_SIZE 128
#define BMC_STAGING_LENGTH 0x2000000
#define CPLD_STAGING_LENGTH 0x100000
#define BB_CPLD_CTRL_BUS "/dev/i2c-12"
#define NIC_CPLD_CTRL_BUS "/dev/i2c-9"
#define CPLD_SLAVE_ADDR 0x80
#define CPLD_INTENT_CTRL_ADDR 0x70
#define CPLD_INTENT_CTRL_OFFSET 0x13
#define BMC_INTENT_VALUE 0x48
#define BMC_INTENT_RCVY_VALUE 0x10
#define CPLD_INTENT_VALUE 0x04
#define CPLD_INTENT_RCVY_VALUE 0x20
#define MUX_SWITCH_CPLD 0x07

class BmcCpldCapsuleComponent : public Component {
  uint8_t pld_type;
  uint8_t bus;
  uint8_t addr;
  public:
    BmcCpldCapsuleComponent(string fru, string comp, uint8_t type, uint8_t _bus, uint8_t _addr)
      : Component(fru, comp), pld_type(type), bus(_bus), addr(_addr) {}
    int print_version();
    int update(string image);
    int fupdate(string image);
};