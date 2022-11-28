#include <stdio.h>
#include <syslog.h>
#include <sys/file.h>
#include <openbmc/obmc-i2c.h>
#include <openbmc/misc-utils.h>
#include "pex88000.h"

static uint8_t fw_active_region = 0;

static void int32_to_char(uint32_t version, char* ver) {

  uint8_t vers[4] = {0};

  vers[0] = version;
  vers[1] = version >> 8;
  vers[2] = version >> 16;
  vers[3] = version >> 24;
  snprintf(ver, MAX_VALUE_LEN, "0x%02X%02X%02X%02X",
   	vers[0], vers[1], vers[2], vers[3]);
}

static uint32_t swap32(uint32_t *swap_in) {
  if (!swap_in)
    return 0;
  uint32_t swap_out = ((*swap_in >> 24) & 0xFF) |
          ((*swap_in >> 8) & 0xFF00) |
          ((*swap_in << 8) & 0xFF0000) |
          ((*swap_in << 24) & 0xFF000000);
  *swap_in = swap_out;
  return swap_out;
}

static void pex88000_i2c_full_encode(uint32_t oft, uint8_t be, uint8_t cmd, uint8_t *rbuf) {
  uint8_t oft17_13bit = (oft >> 13) & 0x1F;
  uint8_t oft12bit = (oft >> 12) & 0x1;
  uint8_t oft11_10bit = (oft >> 10) & 0x3;
  rbuf[0] = cmd;
  rbuf[1] = (0x40 | oft17_13bit);
  rbuf[2] = ((oft12bit << 7) | (be << 2) | oft11_10bit);
  rbuf[3] = (oft >> 2) & 0xFF;
}

static uint8_t set_axi_register_to_full_mode(uint8_t bus, uint8_t addr) {
  uint8_t cmd[8];
  int dev;
  uint8_t rbuf[8];
  char dev_i2c[16];
  cmd[0] = 0x03;
  cmd[1] = 0x00;
  cmd[2] = 0x3C;
  cmd[3] = 0xB3;
  cmd[4] = 0x00;
  cmd[5] = 0x00;
  cmd[6] = 0x00;
  cmd[7] = 0x07;
  sprintf(dev_i2c, "/dev/i2c-%d", bus);
  dev = open(dev_i2c, O_RDWR);
  if (dev < 0) {
    return 0;
  }
  if (retry_cond(!i2c_rdwr_msg_transfer(dev, addr, cmd, sizeof(cmd), rbuf, 0), 3, 0)) {
    syslog(LOG_WARNING, "bus %d set AXI register to full mode failed!\n", bus);
    close(dev);
    return 0;
  }
  close(dev);
  return 1;
}

static uint8_t pex88000_i2c_read(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *resp, uint16_t resp_len) {
  uint8_t cmd[4] = {0};
  int dev;
  char dev_i2c[16];
  pex88000_i2c_full_encode(oft, 0xf, BRCM_I2C5_CMD_READ, cmd);
  sprintf(dev_i2c, "/dev/i2c-%d", bus);
  dev = open(dev_i2c, O_RDWR);
  if (dev < 0) {
    return 0;
  }
  if (i2c_rdwr_msg_transfer(dev, addr, (uint8_t *)&cmd, sizeof(cmd), resp, resp_len)) {
    syslog(LOG_WARNING, "pex88000 read failed!!\n");
    close(dev);
    return 0;
  }
  close(dev);
  return 1;
}

static uint8_t pex88000_i2c_write(uint8_t bus, uint8_t addr, uint32_t oft, uint8_t *data, uint8_t data_len) {
  uint8_t cmd[4] = {0};
  int dev;
  uint8_t ret = 1;
  uint8_t rbuf[8];
  char dev_i2c[16];
  pex88000_i2c_full_encode(oft, 0xf, BRCM_I2C5_CMD_WRITE, cmd);
  sprintf(dev_i2c, "/dev/i2c-%d", bus);
  dev = open(dev_i2c, O_RDWR);
  if (dev < 0) {
    return 0;
  }
  /* This buffer is for PEC calculated and write to switch by I2C */
  uint8_t *data_buf = (uint8_t *)malloc(sizeof(cmd) + data_len);
  if (!data_buf)
    return 0;
  memcpy(data_buf, &cmd, sizeof(cmd));
  memcpy(data_buf + sizeof(cmd), data, data_len);
  if (retry_cond(!i2c_rdwr_msg_transfer(dev, addr, data_buf, sizeof(cmd) + data_len, rbuf, 0), 3, 0)) {
    syslog(LOG_WARNING, "pex88000 write failed!!\n");
    ret = 0;
  }
  free(data_buf);
  close(dev);
  return ret;
}

static uint8_t pend_for_read_valid(uint8_t bus, uint8_t addr) {
  uint32_t resp = 0;
  if (retry_cond(
      pex88000_i2c_read(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&resp, sizeof(resp)) &&
      ((resp >> 24) & 0x8) != 0,
    MAX_RETRY_TIMES, 0)) {
    syslog(LOG_WARNING, "pend_for_read_valid failed after all retries!!\n");
    return 0;
  }
  return 1;
}

static uint8_t pex88000_chime_to_axi_read(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp) {
  uint32_t data = oft;
  swap32(&data);
  if(!pex88000_i2c_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&data, sizeof(data))){
    return 0;
  }
  data = 0x2;
  swap32(&data);
  if(!pex88000_i2c_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&data, sizeof(data))){
    return 0;
  }
  if (!pend_for_read_valid(bus, addr)) {
    syslog(LOG_WARNING, "read data invaild\n");
    return 0;
  }
  if(!pex88000_i2c_read(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)resp, sizeof(resp))){
    return 0;
  }
  return 1;
}

static uint8_t pex88000_chime_to_axi_write(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data) {
  uint32_t wbuf = oft;
  swap32(&wbuf);
  if(!pex88000_i2c_write(bus, addr, BRCM_CHIME_AXI_CSR_ADDR, (uint8_t *)&wbuf, sizeof(wbuf))){
    return 0;
  }
  wbuf = data;
  swap32(&wbuf);
  if(!pex88000_i2c_write(bus, addr, BRCM_CHIME_AXI_CSR_DATA, (uint8_t *)&wbuf, sizeof(wbuf))){
    return 0;
  }
  wbuf = 0x1;
  swap32(&wbuf);
  if(!pex88000_i2c_write(bus, addr, BRCM_CHIME_AXI_CSR_CTL, (uint8_t *)&wbuf, sizeof(wbuf))){
    return 0;
  }
  return 1;
}

uint8_t pex88000_read_spi_control(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t *resp, uint8_t num) {
  uint32_t reg_addr = 0;
  if (num == 2) {
    reg_addr = SPIBASE + oft;
  }
  else if (num == 1){
    reg_addr = SPISHBASE + oft;
  }
  else {
    syslog(LOG_WARNING, "Invalid input number\n");
    return 0;
  }
  if (!pex88000_chime_to_axi_read(bus, addr, reg_addr, resp)){
    syslog(LOG_WARNING, "CHIME to AXI Read SPI register fail!\n");
    return 0;
  }
  return 1;
}

uint8_t pex88000_write_spi_control(uint8_t bus, uint8_t addr, uint32_t oft, uint32_t data, uint8_t num) {
  uint32_t reg_addr = 0;
  if (num == 2) {
    reg_addr = SPIBASE + oft;
  }
  else if (num == 1) {
    reg_addr = SPISHBASE + oft;
  }
  else {
    syslog(LOG_WARNING, "Invalid input number\n");
    return 0;
  }
  if (!pex88000_chime_to_axi_write(bus, addr, reg_addr, data)){
    syslog(LOG_WARNING, "CHIME to AXI Write SPI register fail!\n");
    return 0;
  }
  return 1;
}

static uint8_t pend_for_cmd_be_sent(uint8_t bus, uint8_t addr) {
  uint32_t regvalue = 0xFFFFFFFF;
  if (retry_cond(pex88000_read_spi_control(bus, addr, 0x84, &regvalue, 2) == 1 && (swap32(&regvalue) & 0x00010000) == 0, MAX_RETRY_TIMES, 0)) {
    syslog(LOG_WARNING, "bus %d Waiting for Command to Be Sent failed after all retries\n",bus);
    return 0;
  }
  return 1;
}

uint8_t pex88000_read_spi(uint8_t bus, uint8_t addr, uint32_t offset, uint8_t csel, uint32_t *resp) {
  uint32_t cmd, manualcmd;
  if (!set_axi_register_to_full_mode(bus, addr)){
    return 0;
  }
  cmd = (0x3 << 24) | (offset & 0xffffff);
  pex88000_write_spi_control(bus, addr, 0x80, cmd, 2);
  manualcmd = (0x00013020 & 0xfffffc7f) | (csel << 7);
  pex88000_write_spi_control(bus, addr, 0x84, manualcmd, 2);
  if ( !pend_for_cmd_be_sent(bus, addr)){
    return 0;
  }
  manualcmd = (0x00010820 & 0xfffffc7f) | (csel << 7);
  pex88000_write_spi_control(bus, addr, 0x84, 0x00010820, 2);
  if ( !pend_for_cmd_be_sent(bus, addr)) {
    return 0;
  }
  pex88000_read_spi_control(bus, addr, 0x78, resp, 2);
  return 1;
}

uint8_t pex88000_get_fw_active_region(uint8_t bus, uint8_t addr) {
  uint32_t oft;
  uint32_t header = 0;
  uint32_t region1_status = 0;
  uint32_t region2_status = 0;
  //get fw status header
  oft = BRCM_SPI_FW_REGION1 + BRCM_FW_REGION_STATUS_HEADER_OFT;
  if (!pex88000_read_spi(bus, addr, oft , 0, &header)){
    syslog(LOG_WARNING, "get fw status header fail!\n");
    return 0;
  }
  if(header == 0xFFFFFFFF || header == 0x0) {
    region1_status = 0;
  } else {
    oft = BRCM_SPI_FW_REGION1 + header + BRCM_FW_REGION_CURRENT_STATUS_OFT;
    if (!pex88000_read_spi(bus, addr, oft , 0, &region1_status)){
      syslog(LOG_WARNING, "get fw region1 status fail!\n");
      return 0;
    }
  }

  oft = BRCM_SPI_FW_REGION2 + BRCM_FW_REGION_STATUS_HEADER_OFT;
  if (!pex88000_read_spi(bus, addr, oft , 0, &header)){
    syslog(LOG_WARNING, "get fw status header fail!\n");
    return 0;
  }
  if(header == 0xFFFFFFFF || header == 0x0) {
    region2_status = 0;
  } else {
    oft = BRCM_SPI_FW_REGION2 + header + BRCM_FW_REGION_CURRENT_STATUS_OFT;
    if (!pex88000_read_spi(bus, addr, oft , 0, &region2_status)){
      syslog(LOG_WARNING, "get fw region2 status fail!\n");
      return 0;
    }
  }

  if(region1_status == 0xFFFFFFFF) {
    fw_active_region = 1;
  }
  else if(region2_status == 0xFFFFFFFF) {
    fw_active_region = 2;
  }
  else {
    fw_active_region = 0;
    return 0;
  }
  return 1;
}

uint8_t pex88000_get_main_version(uint8_t bus, char *ver) {
  uint32_t version;
  uint8_t addr = 0xb2;

  if (!set_axi_register_to_full_mode(bus, addr)){
    return 0;
  }
  if (!pex88000_chime_to_axi_read(bus, addr, BRCM_MAIN_VERSION_REG, &version)){
    syslog(LOG_WARNING, "get config version fail!\n");
    return 0;
  }

  int32_to_char(version, ver);
  return 1;
}

uint8_t pex88000_get_sbr_version(uint8_t bus, char *ver) {
  uint32_t version;
  uint8_t addr = 0xb2;

  if (!set_axi_register_to_full_mode(bus, addr)){
    return 0;
  }
  if (!pex88000_chime_to_axi_read(bus, addr, BRCM_SBR_VERSION_REG, &version)){
    syslog(LOG_WARNING, "get config version fail!\n");
    return 0;
  }

  int32_to_char(version, ver);
  return 1;
}

uint8_t pex88000_get_MFG_config_version(uint8_t bus, char *ver) {
  uint32_t version;
  uint8_t addr = 0xb2;

  if (!set_axi_register_to_full_mode(bus, addr)){
    return 0;
  }
  if (!pex88000_chime_to_axi_read(bus, addr, BRCM_MFG_VERSION_REG, &version)){
    syslog(LOG_WARNING, "get config version fail!\n");
    return 0;
  }

  int32_to_char(version, ver);
  return 1;
}
