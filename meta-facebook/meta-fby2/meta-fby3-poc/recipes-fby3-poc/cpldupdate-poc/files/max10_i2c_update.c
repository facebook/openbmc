#include <stdio.h>
#include <stdint.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <openbmc/obmc-i2c.h>
#include <facebook/bic.h>
#include "max10_i2c_update.h"

#define SET_READ_DATA(buf, reg, offset) \
                     do { \
                       buf[offset++] = (reg >> 24) & 0xff; \
                       buf[offset++] = (reg >> 16) & 0xff; \
                       buf[offset++] = (reg >> 8) & 0xff; \
                       buf[offset++] = (reg >> 0) & 0xff; \
                     } while(0)\

#define SET_WRITE_DATA(buf, reg, value, offset) \
                     do { \
                       buf[offset++] = (reg >> 24) & 0xff; \
                       buf[offset++] = (reg >> 16) & 0xff; \
                       buf[offset++] = (reg >> 8)  & 0xff; \
                       buf[offset++] = (reg >> 0)  & 0xff; \
                       buf[offset++] = (value >> 0 )  & 0xFF; \
                       buf[offset++] = (value >> 8 )  & 0xFF; \
                       buf[offset++] = (value >> 16 ) & 0xFF; \
                       buf[offset++] = (value >> 24 ) & 0xFF; \
                    } while(0)\

#define GET_VALUE(buf) (buf[0] << 0) | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24)
#include <time.h>
static int g_i2c_file = 0;
static uint8_t g_cpld_addr = 0;
static bool g_bypass_update = false;

#define DEBUGMSG(X)
//#define DEBUGMSG(X)  printf X

// status register
#define BUSY_IDLE         0x00
#define BUSY_ERASE        0x01
#define BUSY_WRITE        0x02
#define BUSY_READ         0x03

#define READ_SUCCESS      0x04
#define WRITE_SUCCESS     0x08
#define ERASE_SUCCESS     0x10

#define MAX_RETRY         3

void Max10Update_Init(bool bypass_update, int slot_id)
{
  if ( false == bypass_update ) {
    if ( (g_i2c_file = open("/dev/i2c-9", O_RDWR)) < 0 ) {
    }
  } else {
    g_i2c_file = slot_id;
  }

  g_cpld_addr = 0x80;
  g_bypass_update = bypass_update;
}

// MAX 10 NEEK board (10M50) / project
#define ON_CHIP_FLASH_IP_DATA_REG         0x00000000
#define ON_CHIP_FLASH_IP_CSR_STATUS_REG   0x00200020
#define ON_CHIP_FLASH_IP_CSR_CTRL_REG     0x00200024
#define ON_CHIP_FLASH_USER_VER            0x00200028
#define DUAL_BOOT_IP_BASE                 0x00200000

int set_register_via_bypass(int file, uint8_t addr, int reg, int val) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[16] = {0x05/*bus*/, 0x00/*addr*/, 0x00/*read cnt*/};
  uint8_t rxbuf[16] = {0};
  uint8_t txlen = 3; //start from 3
  uint8_t rxlen = 0;

  txbuf[1] = addr;

  SET_WRITE_DATA(txbuf, reg, val, txlen);

  while ( retries > 0 ) {
    ret = bic_ipmb_wrapper((uint8_t)file, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen);
    if (ret) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
     printf("%s() write register fails after retry %d times. ret=%d \n", __func__, MAX_RETRY, ret);
  }
  return ret;
}

int get_register_via_bypass(int file, uint8_t addr, int reg, int *val) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[16] = {0x05/*bus*/, 0x00/*addr*/, 0x04/*read cnt*/};
  uint8_t rxbuf[16] = {0};
  uint8_t txlen = 3; //start from 3
  uint8_t rxlen = 0;

  txbuf[1] = addr;

  SET_READ_DATA(txbuf, reg, txlen);

  while ( retries > 0 ) {
    ret = bic_ipmb_wrapper((uint8_t)file, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, &rxlen);
    if (ret) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
    printf("%s() get register fails after retry %d times. ret=%d\n", __func__, MAX_RETRY, ret);
  }

  *val = GET_VALUE(rxbuf);
  return ret;
}

int set_register_via_i2c(int file, uint8_t addr, int reg, int val) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[8] = {0};
  uint8_t rxbuf[8] = {0};
  uint8_t txlen = 0;
  uint8_t rxlen = 0;

  SET_WRITE_DATA(txbuf, reg, val, txlen);

  while ( retries > 0 ) {
    ret = i2c_rdwr_msg_transfer(file, addr, txbuf, txlen, rxbuf, rxlen);
    if ( ret ) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
    printf("%s() write register fails after retry %d times. ret=%d\n", __func__, MAX_RETRY, ret);
  }

  return ret;
}

int get_register_via_i2c(int file, uint8_t addr, int reg, int *val) {
  int ret;
  int retries = MAX_RETRY;
  uint8_t txbuf[8] = {0};
  uint8_t rxbuf[8] = {0};
  uint8_t txlen = 0;
  uint8_t rxlen = 4;

  SET_READ_DATA(txbuf, reg, txlen);

  while ( retries > 0 ) {
    ret = i2c_rdwr_msg_transfer(file, addr, txbuf, txlen, rxbuf, rxlen);
    if ( ret ) {
      sleep(1);
      retries--;
    } else {
      break;
    }
  }

  if ( retries == 0 ) {
    printf("%s() get register fails after retry %d times. ret=%d\n", __func__, MAX_RETRY, ret);
  }

  *val = GET_VALUE(rxbuf);
  return ret;
}

int WriteRegister(int address, int data)
{
  int ret;

  DEBUGMSG(("Enter %s()\n", __func__));
  if ( true == g_bypass_update ) {
    /*remote update*/
    ret = set_register_via_bypass(g_i2c_file, g_cpld_addr, address, data);
  } else {
    /*update the local cpld via i2c*/
    if ( 0 == g_i2c_file ) {
      printf("i2c device not specified.\n");
      return -1;
    }

    ret = set_register_via_i2c(g_i2c_file, g_cpld_addr, address, data);
  }

  return ret;
}


int ReadRegister(int address)
{
  int ret;
  int data;

  DEBUGMSG(("Enter %s()\n", __func__));
  if ( true == g_bypass_update ) {
    /*remote update*/
    ret = get_register_via_bypass(g_i2c_file, g_cpld_addr, address, &data);
  } else {
    /*update the local cpld via i2c*/
    if ( 0 == g_i2c_file ) {
      printf("i2c device not specified.\n");
      return -1;
    }

    ret = get_register_via_i2c(g_i2c_file, g_cpld_addr, address, &data);
    if ( ret ) {
      printf("get_register_via_i2c fail\n");
    }
  }

  return data;
}

enum{
  WP_CFM0 = 0x1<<27,
  WP_CFM1 = 0x1<<26,
  WP_CFM2 = 0x1<<25,
  WP_UFM0 = 0x1<<24,
  WP_UFM1 = 0x1<<23,
};

int Max10_protectSectors(void)
{
  int ret = 0;
  int value;

  DEBUGMSG(("Enter Max10_protectSectors()\n"));

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X.\n", value));

  value |= (0x1F<<23);
  DEBUGMSG(("write ctrl reg = 0x%08X.\n", value));
  ret = WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X.\n", value));

  DEBUGMSG(("Exit Max10_protectSectors().\n"));
  return ret;
}

int Max10_UnprotectSector(SectorType_t secType)
{
  int ret = 0;
  int value;

  DEBUGMSG(("Enter Max10_UnprotectSector(). \n"));

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X. \n", value));

  switch(secType)
  {
    case Sector_CFM0:
      value = value & (~WP_CFM0);  // set to zero
      break;
    case Sector_CFM1:
      value = value & (~WP_CFM1);
      break;
    case Sector_CFM2:
      value = value & (~WP_CFM2);
      break;
    case Sector_UFM0:
      value = value & (~WP_UFM0);
      break;
    case Sector_UFM1:
      value = value & (~WP_UFM1);
      break;
    case Sector_NotSet:
      value = 0xFFFFFFFF;
      break;
  }

  DEBUGMSG(("write ctrl reg = 0x%08X.\n", value));
  ret = WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X.\n", value));

  DEBUGMSG(("Exit Max10_UnprotectSector(). \n"));
  return ret;
}

int Max10_Erase_Sector(SectorType_t secType)
{
  int ret = 0;
  int value;
  int status;
  int done = 0;

  //Control register bit 20~22
  enum{
    Sector_erase_CFM0	= 0b101 << 20,
    Sector_erase_CFM1	= 0b100 << 20,
    Sector_erase_CFM2	= 0b011 << 20,
    Sector_erase_UFM0	= 0b010 << 20,
    Sector_erase_UFM1	= 0b001 << 20,
    Sector_erase_NotSet     = 0b111 << 20,
  };

  DEBUGMSG(("Enter Max10_Erase_Sector().\n"));

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X. \n", value));

  value &= ~(0x7<<20);  // clear bit 20~22.

  switch(secType)
  {
    case Sector_CFM0:
      value |= Sector_erase_CFM0;
      break;
    case Sector_CFM1:
      value |= Sector_erase_CFM1;
      break;
    case Sector_CFM2:
      value |= Sector_erase_CFM2;
      break;
    case Sector_UFM0:
      value |= Sector_erase_UFM0;
      break;
    case Sector_UFM1:
      value |= Sector_erase_UFM1;
      break;
    case Sector_NotSet:
      value |= Sector_erase_NotSet;
      break;
  }

  WriteRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG, value);
  DEBUGMSG(("write ctrl reg = 0x%08X. \n", value));

  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_CTRL_REG);
  DEBUGMSG(("read ctrl reg = 0x%08X. \n", value));

  //Wait erase finished.
  if(secType != Sector_NotSet) {
    while(!done) {
      status = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);
      DEBUGMSG(("read status reg = 0x%08X. \n", status));

      if( status & BUSY_ERASE) {
        usleep(500*1000);
        printf("Erase sector BUSY. \n");
        continue;
      }

      if( status & ERASE_SUCCESS) {
        done = 1;
        printf("Erase sector SUCCESS. \n");
      } else {
        printf("Erase sector FAIL. Is the device read only?\n");
        ret = -1;
        sleep(1);
      }
    }
  }

  return ret;
}

// write 4 byte into on-chip flash.
int Max10_WriteFlashData(int address, int data)
{
  int ret = 0;

  DEBUGMSG(("%s() write address: %x, data:%x\n", __func__, ON_CHIP_FLASH_IP_DATA_REG + address, data));
  ret = WriteRegister(ON_CHIP_FLASH_IP_DATA_REG + address, data);
  return ret;
}

// Read 4 byte from on-chip flash.
int Max10_ReadFlashData(int address)
{
  int ret = 0;
  ret = ReadRegister(ON_CHIP_FLASH_IP_DATA_REG + address);
  return ret;
}

int Max10_OnChipFlash_GetStatus(void)
{
  int ret = 0;
  ret = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);
  //printf("%s() GetStatus: 0x%X\n", __func__, ret);
  return ret;
}

int Max10_ShowChipVersion(void)
{
  int ret = 0;
  ret = ReadRegister(ON_CHIP_FLASH_USER_VER);
  return ret;
}

int IsBusy(void)
{
  int value;
 
  value = ReadRegister(ON_CHIP_FLASH_IP_CSR_STATUS_REG);

  if( value & (BUSY_ERASE|BUSY_READ|BUSY_WRITE) ) {
    return 1;
  }

  return 0; //timeout or unexpected result
}

int Max10_Update_Rpd(unsigned char *rpd_file, SectorType_t sectorType )
{

#define cfm_start_addr 0x64000
#define cfm_end_addr   0xbffff
  int ret = 0;
  int address;
  int data;
  int receivedHex[4];
  int byte;
  int bContinue = 1;
  int status;
  int offset;
  int retries = MAX_RETRY;
  struct timespec req_tv;
  struct timespec res_tv;
  double temp=0;

  printf("OnChip Flash Status = 0x%X.\n", Max10_OnChipFlash_GetStatus() );

  // State 2: Write Control Register of On-Chip Flash IP to un-protect and erase operation
  if(sectorType == Sector_CFM1 || sectorType == Sector_CFM2) {
    Max10_UnprotectSector(Sector_CFM1);
    Max10_UnprotectSector(Sector_CFM2);
  } else {
    //CFM0
    Max10_UnprotectSector(sectorType);
  }

  // State 3: Erase CFM0, CFM1, CFM2
  if(sectorType == Sector_CFM1 || sectorType == Sector_CFM2) {
    Max10_Erase_Sector(Sector_CFM1);
    Max10_Erase_Sector(Sector_CFM2);
  } else {
    ret = Max10_Erase_Sector(sectorType);
    printf("Max10_Erase_Sector(), ret = %d. \n", ret);
  }

  if(ret != 0) {
    printf("Erase sector fail. \n");
    return -1;
  }

  // Set erase sector to 'none'.
  ret = Max10_Erase_Sector(Sector_NotSet);
  printf("Max10_protectSectors(Sector_NotSet), ret = %d. \n", ret);
  DEBUGMSG(("Start to program...\n", address));

  // State 4: start program
  offset = 0;
  for(address = cfm_start_addr; address < cfm_end_addr; address += 4) {
    /*Get 4 bytes from UART Terminal*/
    receivedHex[0] = rpd_file[offset + 0];
    receivedHex[1] = rpd_file[offset + 1];
    receivedHex[2] = rpd_file[offset + 2];
    receivedHex[3] = rpd_file[offset + 3];

    /*Swap LSB with MSB before write into CFM*/
    for(byte=0; byte<4; byte++) {
      receivedHex[byte] = (((receivedHex[byte] & 0xaa)>>1)|((receivedHex[byte] & 0x55)<<1));
      receivedHex[byte] = (((receivedHex[byte] & 0xcc)>>2)|((receivedHex[byte] & 0x33)<<2));
      receivedHex[byte] = (((receivedHex[byte] & 0xf0)>>4)|((receivedHex[byte] & 0x0f)<<4));
    }

    /*Combine 4 bytes to become 1 word before write operation*/
    data = (receivedHex[0]<<24)|(receivedHex[1]<<16)|(receivedHex[2]<<8)|(receivedHex[3]);

    DEBUGMSG(("Send 0x%0X...", data));
    /*Command to write into On-Chip Flash IP*/
    //usleep(10*1000);

    clock_gettime(CLOCK_REALTIME, &req_tv);
    Max10_WriteFlashData(address, data);
    usleep(50); //tony set 50
    clock_gettime(CLOCK_REALTIME, &res_tv);
    temp =  (res_tv.tv_sec - req_tv.tv_sec -1) *1000 + (1000 + (float) ((res_tv.tv_nsec - req_tv.tv_nsec)/1000000) );
    //usleep(500*1000);
    DEBUGMSG(("Done....%f ms\n", temp));
    // Wait for write operation
    bContinue = 1;

#define STATUS_BIT_MASK  0x1F
    while( bContinue ) {
      status = Max10_OnChipFlash_GetStatus();
      status &= STATUS_BIT_MASK;
      DEBUGMSG(("Staus 0x%0X... retries:%d\n", status, retries));
      if(  (status & BUSY_WRITE) == BUSY_WRITE ) {
        DEBUGMSG(("0ffset 0x%0X, BUSY WRITE... \n", address));
        continue;
      }

      if( (status & WRITE_SUCCESS) == WRITE_SUCCESS) {
        DEBUGMSG(("0ffset 0x%0X, SUCCESS... \n", address));
        bContinue = 0;
        continue;
      }

      if(status != BUSY_IDLE) {
        printf("Write to addr: 0x%X failed. status = 0x%X. \n", address, status);
	bContinue = 0;
        continue;
      }

      if ( status == 0x0 && retries > 0) {
        retries--;
      } else {
        break;
      }
    }

    if ( retries == 0 ) break;

#if 1
    // show percentage
    {
      static int last_percent = 0;
      int total = cfm_end_addr - cfm_start_addr;
      int percent = ((address - cfm_start_addr)*100)/total;
      if(last_percent != percent) {
        last_percent = percent;
	printf(" Prograss  %d %%.  addr: 0x%X \n", percent, address);
      }
    }
#endif

    offset+= 4;
  }

  if ( retries != 0 ) {
    //Re-protect the sector
    Max10_protectSectors();
  } else {
    printf("%s() cannot get the status after retry %d times.\n", __func__, MAX_RETRY);
  }

  if ( g_bypass_update == false) {
    if ( g_i2c_file > 0 ) close(g_i2c_file);
  }

  return ret;
}

void Max10_Show_Ver(void) {
  printf("CPLD Ver: 0x%X\n", Max10_ShowChipVersion());
  if ( g_bypass_update == false) {
    if ( g_i2c_file > 0 ) close(g_i2c_file);
  }
}
