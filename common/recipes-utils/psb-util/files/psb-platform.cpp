#include <openbmc/pal.h>
#include <algorithm>

#ifndef PSB_EEPROM_BUS
#define PSB_EEPROM_BUS 0x05
#endif

uint8_t plat_num_cpu(uint8_t /* fru_id */) {
  return 1;
}

int plat_read_psb_eeprom(uint8_t fru, uint8_t /* cpu */, uint8_t *rbuf, size_t rsize) {
  int ret;
  std::fill(rbuf, rbuf + rsize, 0);
  uint8_t tbuf[16] = {0};
  uint8_t tlen = 0;

  tbuf[0] = PSB_EEPROM_BUS;             // bus
  tbuf[1] = 0xA0;                       // dev addr
  tbuf[2] = rsize;
  tbuf[3] = 0x00;                       // write data addr (MSB)
  tbuf[4] = 0x00;                       // write data addr (LSB)
  tlen = 5;

  uint8_t rlen = 0;
  ret = bic_ipmb_wrapper(fru, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, tbuf, tlen, rbuf, &rlen);
  if (ret != 0) {
    return ret;
  }
  if ((size_t)rlen != rsize) {
    return -1;
  }
  return 0;
}
