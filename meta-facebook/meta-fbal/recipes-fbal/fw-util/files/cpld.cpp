#include <cstdio>
#include <cstring>
#include <openbmc/obmc-i2c.h>
#include <openbmc/pal.h>
#include <openbmc/cpld.h>
#include "fw-util.h"

using namespace std;

// According to QSYS setting in FPGA project

// on-chip Flash IP
#define ON_CHIP_FLASH_IP_CSR_BASE        (0x00100020)
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG  (ON_CHIP_FLASH_IP_CSR_BASE + 0x0)
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG    (ON_CHIP_FLASH_IP_CSR_BASE + 0x4)

#define ON_CHIP_FLASH_IP_DATA_REG        (0x00000000)
// Dual-boot IP
#define DUAL_BOOT_IP_BASE                (0x00100000)

#define CFM0_10M25_START_ADDR            (0x00064000)
#define CFM0_10M25_END_ADDR              (0x000BFFFF)
#define CFM1_10M25_START_ADDR            (0x00008000)
#define CFM1_10M25_END_ADDR              (0x00063FFF)

#define CFM0_10M16_START_ADDR            (0x0004A000)
#define CFM0_10M16_END_ADDR              (0x0008BFFF)
#define CFM1_10M16_START_ADDR            (0x00008000)
#define CFM1_10M16_END_ADDR              (0x00049FFF)

enum {
  CFM_IMAGE_NONE = 0,
  CFM_IMAGE_1,
  CFM_IMAGE_2,
};

enum {
  CFM0_10M25 = 0,
  CFM1_10M25,
  CFM0_10M16,
  CFM1_10M16
};

static altera_max10_attr_t s_attrs[] = {
  [CFM0_10M25] = {
    .img_type = CFM_IMAGE_1,
    .start_addr = CFM0_10M25_START_ADDR,
    .end_addr = CFM0_10M25_END_ADDR,
  },
  [CFM1_10M25] = {
    .img_type = CFM_IMAGE_2,
    .start_addr = CFM1_10M25_START_ADDR,
    .end_addr = CFM1_10M25_END_ADDR,
  },
  [CFM0_10M16] = {
    .img_type = CFM_IMAGE_1,
    .start_addr = CFM0_10M16_START_ADDR,
    .end_addr = CFM0_10M16_END_ADDR,
  },
  [CFM1_10M16] = {
    .img_type = CFM_IMAGE_2,
    .start_addr = CFM1_10M16_START_ADDR,
    .end_addr = CFM1_10M16_END_ADDR,
  }
};

class CpldComponent : public Component {
  string pld_name;
  uint8_t pld_type;
  altera_max10_attr_t attr;
  int _update(const char *path, uint8_t is_signed);
  public:
    CpldComponent(string fru, string comp, string name, uint8_t type, uint8_t ctype, uint8_t bus, uint8_t addr)
      : Component(fru, comp), pld_name(name), pld_type(type),
        attr{bus, addr, s_attrs[ctype].img_type, s_attrs[ctype].start_addr, s_attrs[ctype].end_addr,
             ON_CHIP_FLASH_IP_CSR_BASE, ON_CHIP_FLASH_IP_DATA_REG, DUAL_BOOT_IP_BASE} {}
    int update(string image);
    int fupdate(string image);
    int print_version();
};

int CpldComponent::_update(const char *path, uint8_t is_signed) {
  int ret = -1;
  uint8_t i, cfm_cnt = 1, rev_id = 0xF;

  if ((pal_is_pfr_active() == PFR_ACTIVE) && (pld_type == MAX10_10M25)) {
    string dev;
    string cmd;

    if (!sys.get_mtd_name(string("stg-cpld"), dev)) {
      return FW_STATUS_FAILURE;
    }

    sys.output << "Flashing to device: " << dev << endl;
    cmd = "flashcp -v " + string(path) + " " + dev;
    ret = sys.runcmd(cmd);
    return pal_fw_update_finished(0, _component.c_str(), ret);
  }

  if ((pld_type == MAX10_10M25) && (!pal_get_board_rev_id(&rev_id) && (rev_id < 2))) {
    cfm_cnt = 2;
  }

  for (i = 0; i < cfm_cnt; i++) {
    if (i == 1) {
      // workaround for EVT boards that CONFIG_SEL of main CPLD is floating,
      // so program both CFMs
      attr.img_type = s_attrs[CFM1_10M25].img_type;
      attr.start_addr = s_attrs[CFM1_10M25].start_addr;
      attr.end_addr = s_attrs[CFM1_10M25].end_addr;
    }

    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      printf("Cannot open i2c!\n");
      return -1;
    }

    ret = cpld_program((char *)path, (char *)pld_name.substr(0, 4).c_str(), is_signed);
    cpld_intf_close(INTF_I2C);
    if (ret) {
      printf("Error Occur at updating CPLD FW!\n");
      break;
    }
  }

  return ret;
}

int CpldComponent::update(string image) {
  return _update(image.c_str(), 1);
}

int CpldComponent::fupdate(string image) {
  return _update(image.c_str(), 0);
}

int CpldComponent::print_version() {
  int ret = -1;
  int ifd = 0;
  uint8_t i, tbuf[8], rbuf[8], cmds[4] = {0x7f, 0x01, 0x7e, 0x7d};
  uint8_t ver[4];
  char dev_i2c[16];

  if (pal_is_pfr_active() && (pld_type == MAX10_10M25)) {
    do {
      sprintf(dev_i2c, "/dev/i2c-%d", PFR_MAILBOX_BUS);
      ifd = open(dev_i2c, O_RDWR);
      if (ifd < 0) {
        break;
      }

      for (i = 0; i < sizeof(cmds); i++) {
        tbuf[0] = cmds[i];
        if ((ret = i2c_rdwr_msg_transfer(ifd, PFR_MAILBOX_ADDR, tbuf, 1, rbuf, 1))) {
          break;
        }
        ver[i] = rbuf[0];
      }
    } while (0);

    if (ifd > 0) {
      close(ifd);
    }
  } else {
    if (cpld_intf_open(pld_type, INTF_I2C, &attr)) {
      printf("Cannot open i2c!\n");
      return -1;
    }

    ret = cpld_get_ver((uint32_t *)ver);
    cpld_intf_close(INTF_I2C);
  }

  if (ret) {
    printf("%s CPLD Version: NA\n", pld_name.c_str());
  } else {
    printf("%s CPLD Version: v%02u.%02u.%02u.%02u\n", pld_name.c_str(), ver[3], ver[2], ver[1], ver[0]);
  }

  return 0;
}

CpldComponent pfr_cfm0("cpld", "pfr_cpld", "PFR_CPLD", MAX10_10M25, CFM0_10M25, 4, 0x5a);
CpldComponent mod_cpld("cpld", "mod_cpld", "MOD_CPLD", MAX10_10M16, CFM0_10M16, 4, 0x55);
CpldComponent glb_cpld("cpld", "glb_cpld", "GLB_CPLD", MAX10_10M16, CFM0_10M16, 23, 0x55);
