#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>
#include <openbmc/obmc-i2c.h>
#include "bic_cpld_lattice_fwupdate.h"
#include "bic_xfer.h"

#define EXP1_TI_VENDOR_ID 0x1CBF
#define EXP1_TI_PRODUCT_ID 0x0007

#define EXP2_TI_VENDOR_ID 0x1CC0
#define EXP2_TI_PRODUCT_ID 0x0007

#define MAX_RETRY 500
#define LATTICE_COL_SIZE 128

#define USB_PKT_SIZE 0x20

//#define CPLD_DEBUG

typedef struct
{
  unsigned long int QF;
  unsigned int *CF;
  unsigned int CF_Line;
  unsigned int *UFM;
  unsigned int UFM_Line;
  unsigned int Version;
  unsigned int CheckSum;
  unsigned int FEARBits;
  unsigned int FeatureRow;
} CPLDInfo;

enum {
  MUX  = 0xE2,
  CPLD = 0x80,
};

#if 0
static int
_update_fw(uint8_t slot_id, uint8_t target, uint32_t offset, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret = 0;
  int retries = MAX_RETRY;

  // File the IANA ID
  memcpy(tbuf, (uint8_t *)&IANA_ID, 3);

  // Fill the component for which firmware is requested
  tbuf[3] = target;

  tbuf[4] = (offset) & 0xFF;
  tbuf[5] = (offset >> 8) & 0xFF;
  tbuf[6] = (offset >> 16) & 0xFF;
  tbuf[7] = (offset >> 24) & 0xFF;

  tbuf[8] = len & 0xFF;
  tbuf[9] = (len >> 8) & 0xFF;

  memcpy(&tbuf[10], buf, len);

  tlen = len + 10;

  do {
    ret = bic_ipmb_send(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_UPDATE_FW, tbuf, tlen, rbuf, &rlen, intf);
    if ( ret < 0 ) {
      sleep(1);
      printf("_update_fw: slot: %d, target %d, offset: %d, len: %d retrying..\n", slot_id, target, offset, len);
    } else break;
  } while ( retries-- > 0 );

  return ret;
}
#endif

static int
send_cpld_data(uint8_t slot_id, uint8_t intf, uint8_t addr, uint8_t *data, uint8_t data_len, uint8_t *rsp, uint8_t read_cnt) {
  int ret = 0;
  int retries = 3;
  int i2cfd = 0;
  uint8_t txbuf[256] = {0x9c, 0x9c, 0x00, intf, NETFN_APP_REQ << 2, CMD_APP_MASTER_WRITE_READ, 0x01/*bus 1*/, addr, read_cnt/*read cnt*/};
  uint8_t txlen = 9;//start from 9
  uint8_t rxbuf[256] = {0};
  uint8_t rxlen = 0;
  static uint8_t bus = 0xff;

  if (intf != NONE_INTF) {
      if ( bus == 0xff ) {
        if ( REXP_BIC_INTF == intf ) {
          uint8_t board_type = 0;
          ret = fby35_common_get_2ou_board_type(slot_id, &board_type);
          if ( ret < 0 ) {
            syslog(LOG_WARNING, "Failed to get 2ou board type\n");
            return ret;
          }

          if ( board_type == GPV3_MCHP_BOARD ||
               board_type == GPV3_BRCM_BOARD ) {
            bus = 0x13;
          } else {
            bus = 0x01;
          }
        } else {
          bus = 0x01;
        }
      }

      txbuf[6] = bus;

      if ( data_len > 0 ) {
        memcpy(&txbuf[txlen], data, data_len);
        txlen += data_len;
      }

      while ( retries > 0 ) {
        ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, txbuf, txlen, rxbuf, &rxlen);
        if (ret) {
          sleep(1);
          retries--;
        } else {
          break;
        }
      }

      if ( retries == 0 || rxbuf[6] != 0x0 || rxlen < 7 ) {
         //printf("%s() read/write register fails after retry 3 times. ret=%d \n", __func__,  ret);
         return -1;
      }

      if ( read_cnt > 0 ) {
        memcpy(rsp, &rxbuf[7], read_cnt);
      }
  } else {
    i2cfd = i2c_cdev_slave_open(slot_id + SLOT_BUS_BASE, SB_CPLD_ADDRESS_UPDATE, I2C_SLAVE_FORCE_CLAIM);
    if ( i2cfd < 0 ) {
      syslog(LOG_WARNING, "%s() Failed to open %d", __func__, CPLD_ADDRESS);
      return -1;
    }

    txlen = 0;
    memcpy(&txbuf[txlen], data, data_len);
    txlen = data_len;
    rxlen = read_cnt;
    while (retries > 0) {
      ret = i2c_rdwr_msg_transfer(i2cfd, SB_CPLD_ADDRESS_UPDATE << 1, txbuf, txlen, rxbuf, rxlen);
      if ( ret < 0 ) {
        retries--;
        msleep(100);
      } else {
        break;
      }
    }
    if (retries == 0) {
      syslog(LOG_WARNING, "%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d", __func__, txlen);
      if ( i2cfd > 0 ) {
        close(i2cfd);
      }
      return -1;
    }

    if ( read_cnt > 0 ) {
      memcpy(rsp, rxbuf, read_cnt);
    }
  }

  if ( i2cfd > 0 ) {
    close(i2cfd);
  }
  return ret;
}

static int
read_cpld_status_flag(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0x3C, 0x0, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 4;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);

  if ( ret < 0 ) {
    printf("Couldn't read_cpld_status_flag cmd\n");
    return ret;
  }

  rsp[0] = (rsp[2] >> 4) & 0x3;

  ret = (rsp[0] == 0x0)?0:-1;

  return ret;
}

static int
read_cpld_busy_flag(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0xF0, 0x0, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 1;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send read_cpld_busy_flag cmd\n");
    return ret;
  }

  rsp[0] = (rsp[0] & 0x80) >> 7;

  ret = (rsp[0] == 0x0)?0:-1;

  return ret;
}

/*
static int
set_mux_to_cpld(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[1] = {0x2};
  uint8_t data_len = 1;
  ret = send_cpld_data(slot_id, intf, addr, data, data_len, NULL, 0);
  return ret;
}*/

static int
read_cpld_dev_id(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0xE0, 0x0, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 4;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    return ret;
  }

  int i;
  printf("CPLD DevID: ");
  for (i=0;i<read_cnt;i++)
    printf("%02X", rsp[i]);
  printf("\n");

  return ret;
}

static int
enter_transparent_mode(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  int retry = MAX_RETRY;
  uint8_t data[3]= {0x74, 0x08, 0x00};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send enter_transparent_mode cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(50);
    else break;
  } while(  retry-- );

  return ret;
}

static int
erase_flash(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  int retry = MAX_RETRY;
  uint8_t data[4]= {0x0E, 0x04/*erase CF*/, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send erase_flash cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(50);
    else break;
  } while( retry-- );

  retry = MAX_RETRY;
  do {
    ret = read_cpld_status_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(50);
    else break;
  } while( retry-- );

  return ret;
}

static int
reset_config_addr(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0x46, 0x00, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send reset_config_addr cmd\n");
    return ret;
  }

  return ret;
}

#if 1
static int
program_cf(uint8_t slot_id, uint8_t intf, uint8_t addr, uint8_t *cf_data) {
  int ret;
  int retry = MAX_RETRY;
  uint8_t data[20]= {0x70, 0x0, 0x0, 0x01};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;
  int idx;

  for (idx = 4; idx <= 16; idx += 4) {
    data[idx + 0] = cf_data[idx - 1];
    data[idx + 1] = cf_data[idx - 2];
    data[idx + 2] = cf_data[idx - 3];
    data[idx + 3] = cf_data[idx - 4];
  }

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send program_cf cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(10);
    else break;
  } while( retry--);

  return ret;
}
#endif

static int
verify_cf(uint8_t slot_id, uint8_t intf, uint8_t addr, uint8_t *cf_data) {
  int ret;
  int retry = MAX_RETRY;
  uint8_t data[4]= {0x73, 0x00, 0x00, 0x01};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t cmp_data[16] = {0};
  uint8_t read_cnt = sizeof(rsp);
  int idx;

  for (idx = 0; idx <= 12; idx += 4) {
    cmp_data[idx + 0] = cf_data[idx + 3];
    cmp_data[idx + 1] = cf_data[idx + 2];
    cmp_data[idx + 2] = cf_data[idx + 1];
    cmp_data[idx + 3] = cf_data[idx + 0];
  }

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send program_cf cmd\n");
    return ret;
  }

  if ( 0 != memcmp(rsp, cmp_data, read_cnt)) {
    printf("Data verify fail\n");
    return -1;
  }

  do {
    ret = read_cpld_busy_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(10);
    else break;
  } while( retry--);

  return ret;
}

static int
program_user_code(uint8_t slot_id, uint8_t intf, uint8_t addr, uint8_t *user_data) {
  int ret;
  int retry = MAX_RETRY;
  uint8_t data[8]= {0xC2, 0x0, 0x0, 0x00};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  memcpy(&data[4], user_data, 4);
  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send program_user_code cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag(slot_id, intf, addr);
    if ( ret < 0 ) msleep(10);
    else break;
  } while( retry-- );

  return ret;
}

static int
program_done(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0x5E, 0x0, 0x0, 0x00};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send program_done cmd\n");
    return ret;
  }

  return ret;
}

static int
isc_disable(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[3]= {0x26, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send isc_disable cmd\n");
    return ret;
  }

  return ret;
}

#if 0
static uint8_t
read_cpld_user_code(uint8_t slot_id, uint8_t intf, uint8_t addr) {
  int ret;
  uint8_t data[4]= {0xC0, 0x0, 0x0, 0x0};
  uint8_t data_len = sizeof(data);
  uint8_t rsp[16] = {0};
  uint8_t read_cnt = 4;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  int i;
  printf("Usercode: ");
  for (i=0;i<read_cnt;i++)
    printf("%02X", rsp[i]);
  printf("\n");

  return ret;
}
#endif

static int
detect_cpld_dev(uint8_t slot_id, uint8_t intf) {
  int retries = 100;
  int ret = 0;
  do {

    //ret = set_mux_to_cpld(slot_id, intf, MUX);

    ret = read_cpld_dev_id(slot_id, intf, CPLD);

    if ( ret < 0 ) {
      msleep(10);
    } else {
      break;
    }
  } while(retries--);

  return ret;
}

/*reverse byte order*/
static uint8_t
reverse_bit(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

/*search the index of char in string*/
static int
indexof(const char *str, const char *c) {
  char *ptr = strstr(str, c);
  int  index = 0;

  if ( ptr )
  {
    index = ptr - str;
  }
  else
  {
    index = -1;
  }

  return index;
}

/*identify the str start with a specific str or not*/
static int
startWith(const char *str, const char *c) {
  int len = strlen(c);
  int i;

  for ( i=0; i<len; i++ )
  {
    if ( str[i] != c[i] )
    {
       return 0;
    }
  }
  return 1;
}

/*convert bit data to byte data*/
static unsigned int
ShiftData(char *data, unsigned int *result, int len) {
  int i;
  int ret = 0;
  int result_index = 0, data_index = 31;
  int bit_count = 0;

#ifdef CPLD_DEBUG
  printf("[%s][%s]\n", __func__, data);

  for ( i = 0; i < len; i++ )
  {
    printf("%c", data[i]);

    if ( 0 == ((i+1) % 8) )
    {
       printf("\n");
    }
  }
#endif

  for ( i = 0; i < len; i++ )
  {
    data[i] = data[i] - 0x30;

    result[result_index] |= ((uint8_t)data[i] << data_index);

#ifdef CPLD_DEBUG
    printf("[%s]%x %d %x\n", __func__, data[i], data_index, result[result_index]);
#endif
    data_index--;
    bit_count++;

    if ( 0 == ((i+1) % 32) )
    {
      data_index = 31;
#ifdef CPLD_DEBUG
      printf("[%s]%x\n", __func__, result[result_index]);
#endif
      result_index++;
    }
  }

  if ( bit_count != len )
  {
     printf("[%s] Expected Data Length is [%d] but not [%d] ", __func__, bit_count, len);

     ret = -1;
  }

  return ret;
}

static int
LCMXO2Family_Get_Update_Data_Size(FILE *jed_fd, int *cf_size) {
  const char TAG_CF_START[]="L000";
  int ReadLineSize = LATTICE_COL_SIZE + 2;
  char tmp_buf[ReadLineSize];
  unsigned int CFStart = 0;
  int ret = 0;
  fseek(jed_fd, 0, SEEK_SET);

  while( NULL != fgets(tmp_buf, ReadLineSize, jed_fd) )
  {
    if ( startWith(tmp_buf, TAG_CF_START/*"L000"*/) )
    {
      CFStart = 1;
    } else if ( startWith(tmp_buf, "*") ) {
      /* the end of the CF data*/
      break;
    }

    if ( CFStart )
    {
      if ( !startWith(tmp_buf, TAG_CF_START/*"L000"*/) && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          (*cf_size)++;
        }
      }
    }
  }
  //cf must greater than 0
  if ( !(*cf_size) )
  {
    ret = -1;
  }

  return ret;
}

static int
LCMXO2Family_JED_File_Parser(FILE *jed_fd, CPLDInfo *dev_info) {
  /**TAG Information**/
  const char TAG_QF[]="QF";
  const char TAG_CF_START[]="L000";
  const char TAG_UFM[]="NOTE TAG DATA";
  const char TAG_ROW[]="NOTE FEATURE";
  const char TAG_CHECKSUM[]="C";
  const char TAG_USERCODE[]="NOTE User Electronic";
  /**TAG Information**/

  int ReadLineSize = LATTICE_COL_SIZE + 2;//the len of 128 only contain data size, '\n' need to be considered, too.
  char tmp_buf[ReadLineSize];
  char data_buf[LATTICE_COL_SIZE];
  unsigned int CFStart = 0;
  unsigned int UFMStart = 0;
  unsigned int ROWStart = 0;
  unsigned int VersionStart = 0;
  unsigned int ChkSUMStart = 0;
  unsigned int JED_CheckSum = 0;
  int copy_size;
  int current_addr=0;
  int i;
  int ret = 0;
  int cf_size_used = 0; //(cf_size * LATTICE_COL_SIZE) / 8; // unit: bytes

  LCMXO2Family_Get_Update_Data_Size(jed_fd, &cf_size_used);

  //printf("Line cnt: %d\n", cf_size_used);
  cf_size_used = (cf_size_used * LATTICE_COL_SIZE) / 8;
  //printf("Size cnt: %d\n", cf_size_used);

  fseek(jed_fd, 0, SEEK_SET);

  dev_info->CF = (unsigned int*)malloc( cf_size_used );
  memset(dev_info->CF, 0, cf_size_used);

  dev_info->CF_Line=0;
  dev_info->UFM_Line=0;

  while( NULL != fgets(tmp_buf, ReadLineSize, jed_fd) )
  {
    if ( startWith(tmp_buf, TAG_QF/*"QF"*/) )
    {
      copy_size = indexof(tmp_buf, "*") - indexof(tmp_buf, "F") - 1;

      memset(data_buf, 0, sizeof(data_buf));

      memcpy(data_buf, &tmp_buf[2], copy_size );

      dev_info->QF = atol(data_buf);

#ifdef CPLD_DEBUG
      printf("[QF]%lu\n",dev_info->QF);
#endif
    }
    else if ( startWith(tmp_buf, TAG_CF_START/*"L000"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[CFStart]\n");
#endif
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[UFMStart]\n");
#endif
      UFMStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_ROW/*"NOTE FEATURE"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[ROWStart]\n");
#endif
      ROWStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_USERCODE/*"NOTE User Electronic"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[VersionStart]\n");
#endif
      VersionStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_CHECKSUM/*"C"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[ChkSUMStart]\n");
#endif
      ChkSUMStart = 1;
    }

    if ( CFStart )
    {
#ifdef CPLD_DEBUG
      //printf("[%s][%d][%c]", __func__, (int)strlen(tmp_buf), tmp_buf[0]);
#endif
      if ( !startWith(tmp_buf, TAG_CF_START/*"L000"*/) && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          current_addr = (dev_info->CF_Line * LATTICE_COL_SIZE) / 32;

          memset(data_buf, 0, sizeof(data_buf));

          memcpy(data_buf, tmp_buf, LATTICE_COL_SIZE);

          /*convert string to byte data*/
          ShiftData(data_buf, &dev_info->CF[current_addr], LATTICE_COL_SIZE);
#ifdef CPLD_DEBUG
          printf("[%u]%x %x %x %x\n",dev_info->CF_Line, dev_info->CF[current_addr],dev_info->CF[current_addr+1],dev_info->CF[current_addr+2],dev_info->CF[current_addr+3]);
#endif
          //each data has 128bits(4*unsigned int), so the for-loop need to be run 4 times
          for ( i = 0; i < sizeof(unsigned int); i++ )
          {
            JED_CheckSum += reverse_bit((dev_info->CF[current_addr+i]>>24) & 0xff);
            JED_CheckSum += reverse_bit((dev_info->CF[current_addr+i]>>16) & 0xff);
            JED_CheckSum += reverse_bit((dev_info->CF[current_addr+i]>>8)  & 0xff);
            JED_CheckSum += reverse_bit((dev_info->CF[current_addr+i])     & 0xff);
          }

            dev_info->CF_Line++;
        }
        else
        {
#ifdef CPLD_DEBUG
          printf("[%s]CF Line: %u\n", __func__, dev_info->CF_Line);
#endif
          CFStart = 0;
        }
      }
    }
    else if ( ChkSUMStart && strlen(tmp_buf) != 1 )
    {
      ChkSUMStart = 0;

      copy_size = indexof(tmp_buf, "*") - indexof(tmp_buf, "C") - 1;

      memset(data_buf, 0, sizeof(data_buf));

      memcpy(data_buf, &tmp_buf[1], copy_size );

      dev_info->CheckSum = strtoul(data_buf, NULL, 16);
      printf("[ChkSUM]%x\n",dev_info->CheckSum);
    }
    else if ( ROWStart )
    {
       if ( !startWith(tmp_buf, TAG_ROW/*"NOTE FEATURE"*/ ) && strlen(tmp_buf) != 1 )
       {
          if ( startWith(tmp_buf, "E" ) )
          {
            copy_size = strlen(tmp_buf) - indexof(tmp_buf, "E") - 2;

            memset(data_buf, 0, sizeof(data_buf));

            memcpy(data_buf, &tmp_buf[1], copy_size );

            dev_info->FeatureRow = strtoul(data_buf, NULL, 2);
          }
          else
          {
            copy_size = indexof(tmp_buf, "*") - 1;

            memset(data_buf, 0, sizeof(data_buf));

            memcpy(data_buf, &tmp_buf[2], copy_size );

            dev_info->FEARBits = strtoul(data_buf, NULL, 2);
#ifdef CPLD_DEBUG
            printf("[FeatureROW]%x\n", dev_info->FeatureRow);
            printf("[FEARBits]%x\n", dev_info->FEARBits);
#endif
            ROWStart = 0;
          }
       }
    }
    else if ( VersionStart )
    {
      if ( !startWith(tmp_buf, TAG_USERCODE/*"NOTE User Electronic"*/) && strlen(tmp_buf) != 1 )
      {
        VersionStart = 0;

        if ( startWith(tmp_buf, "UH") )
        {
          copy_size = indexof(tmp_buf, "*") - indexof(tmp_buf, "H") - 1;

          memset(data_buf, 0, sizeof(data_buf));

          memcpy(data_buf, &tmp_buf[2], copy_size );

          dev_info->Version = strtoul(data_buf, NULL, 16);
#ifdef CPLD_DEBUG
          printf("[UserCode]%x\n",dev_info->Version);
#endif
        }
      }
    }
    else if ( UFMStart )
    {
      if ( !startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) && !startWith(tmp_buf, "L") && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          current_addr = (dev_info->UFM_Line * LATTICE_COL_SIZE) / 32;

          memset(data_buf, 0, sizeof(data_buf));

          memcpy(data_buf, tmp_buf, LATTICE_COL_SIZE);

          ShiftData(data_buf, &dev_info->UFM[current_addr], LATTICE_COL_SIZE);
#ifdef CPLD_DEBUG
          printf("%x %x %x %x\n",dev_info->UFM[current_addr],dev_info->UFM[current_addr+1],dev_info->UFM[current_addr+2],dev_info->UFM[current_addr+3]);
#endif
          dev_info->UFM_Line++;
        }
        else
        {
#ifdef CPLD_DEBUG
          printf("[%s]UFM Line: %u\n", __func__, dev_info->UFM_Line);
#endif
          UFMStart = 0;
        }
      }
    }
  }

  JED_CheckSum = JED_CheckSum & 0xffff;

  if ( dev_info->CheckSum != JED_CheckSum || dev_info->CheckSum == 0)
  {
    printf("[%s] JED File CheckSum Error\n", __func__);
    ret = -1;
  }
#ifdef CPLD_DEBUG
  else
  {
    printf("[%s] JED File CheckSum OKay\n", __func__);
  }
#endif
  return ret;
}

int update_bic_cpld_lattice(uint8_t slot_id, char *image, uint8_t intf, uint8_t force) {
  int ret = 0;
  uint32_t fsize = 0;
  uint32_t record_offset = 0;
  CPLDInfo dev_info = {0};
  FILE *fp = NULL;
  uint8_t addr = 0xff;
  /*uint8_t buf[16] = {0};
  uint8_t transfer_buf[16] = {0};
  uint32_t offset = 0;
  const uint16_t read_count = 16;
  const uint8_t target = UPDATE_CPLD;*/
  int retry = 0;

  fp = fopen(image, "r");
  if ( fp == NULL ) {
    ret = -1;
    goto error_exit;
  }

  memset(&dev_info, 0, sizeof(dev_info));
  ret = LCMXO2Family_JED_File_Parser(fp, &dev_info);
  if ( ret < 0 ) {
    goto error_exit;
  }

  fsize = dev_info.CF_Line / 20;

  do {
    // step 1 - detect CPLD
    ret = detect_cpld_dev(slot_id, intf);
    if ( ret < 0 ) {
      printf("Device is not found!\n");
      goto error_exit;
    }

    //step 1.5 - provide the addr
    addr = CPLD;

    //step 2 - enter transparent mode and check the status
    ret = enter_transparent_mode(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't enter transparent mode!\n");
      goto error_exit;
    }

    //step 3 - erase the CF and check the status
    ret = erase_flash(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't erase the flash!\n");
      goto error_exit;
    }

    //step 4 - Transmit Reset Configuration Address
    ret = reset_config_addr(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash!\n");
      goto error_exit;
    }

    //step 5 - send data and check the status
    int i, data_idx/*, idx*/; //two vars are used in for-loop
#if 0
    printf("[Update CPLD via Exp BIC]\n");
    for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
      memcpy(buf, (uint8_t *)&dev_info.CF[data_idx], read_count);
      for (idx = 0; idx <= 12; idx+=4) {
          transfer_buf[idx + 0] = buf[idx + 3];
          transfer_buf[idx + 1] = buf[idx + 2];
          transfer_buf[idx + 2] = buf[idx + 1];
          transfer_buf[idx + 3] = buf[idx + 0];
      }

      // printf("[write] offset : %d \n", offset);
      // for (int k = 0; k < read_count; k++) {
      //   printf("%02X ", transfer_buf[k]);
      // }
      // printf("\n");

      ret = _update_fw(slot_id, target, offset, read_count, transfer_buf, intf);
      if ( ret < 0 ) {
        goto error_exit;
      }

      offset += read_count;
      if ( (record_offset + fsize) <= i ) {
        _set_fw_update_ongoing(slot_id, 60);
        printf("updated cpld: %d %%\n", (i/fsize)*5);
        record_offset += fsize;
      }
    }
#else
    for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
      ret = program_cf(slot_id, intf, addr, (uint8_t *)&dev_info.CF[data_idx]);
      if ( ret < 0 ) {
        printf("send cf data but ret < 0. exit.\n");
        goto error_exit;
      }
      if ( (record_offset + fsize) <= i ) {
        printf("updated cpld: %d %%\n", (i/fsize)*5);
        record_offset += fsize;
      }
    }
#endif

    //step 6 - program user code
    ret = program_user_code(slot_id, intf, addr, (uint8_t *)&dev_info.Version);
    if ( ret < 0 ) {
      printf("Couldn't program the usercode!\n");
      goto error_exit;
    }


    //step 7 - verify the CF data
    //step 7.1 Transmit Reset Configuration Address again
    ret = reset_config_addr(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash again!\n");
      goto error_exit;
    }

    record_offset = 0;
    //step 7.2 Transmit Read Command with Number of Pages
    for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
      ret = verify_cf(slot_id, intf, addr, (uint8_t *)&dev_info.CF[data_idx]);
      if ( ret < 0 ) {
        printf("Read cf data but ret < 0. exit\n");
        goto error_exit;//break;
      }

      if ( (record_offset + fsize) <= i ) {
        printf("verify cpld: %d %%\n", (i/fsize)*5);
        record_offset += fsize;
      }
    }

    if (i == dev_info.CF_Line) {
      break;
    }
  } while ((++retry) < RETRY_TIME);

  if (retry == RETRY_TIME) {
    goto error_exit;
  }

  //step 8 - program done
  ret = program_done(slot_id, intf, addr);
  if ( ret < 0 ) {
    printf("Couldn't finish the program!\n");
    goto error_exit;
  }

  //step 9 - Disable configuration interface
  ret = isc_disable(slot_id, intf, addr);
  if ( ret < 0 ) {
    printf("Couldn't disable the configuration interface!\n");
  }

error_exit:

  if ( fp != NULL ) fclose(fp);
  if ( dev_info.CF != NULL ) free(dev_info.CF);

  return ret;
}

int
bic_init_exp_usb_dev(uint8_t slot_id, uint8_t intf, usb_dev* udev)
{
  int ret;
  int index = 0;
  char found = 0;
  ssize_t cnt;
  uint8_t bmc_location = 0;
  int recheck = MAX_CHECK_DEVICE_TIME;
  uint16_t vid = EXP1_TI_VENDOR_ID;
  uint16_t pid = EXP1_TI_PRODUCT_ID;

  ret = libusb_init(NULL);
  if (ret < 0) {
    printf("Failed to initialise libusb\n");
    goto error_exit;
  } else {
    printf("Init libusb Successful!\n");
  }

  if ( intf == FEXP_BIC_INTF) {
    vid = EXP1_TI_VENDOR_ID;
    pid = EXP1_TI_PRODUCT_ID;
  } else if ( intf == REXP_BIC_INTF) {
    vid = EXP2_TI_VENDOR_ID;
    pid = EXP2_TI_PRODUCT_ID;
  } else {
    printf("Unknow update interface\n");
    goto error_exit;
  }

  do {
    cnt = libusb_get_device_list(NULL, &udev->devs);
    if (cnt < 0) {
      printf("There are no USB devices on bus\n");
      goto error_exit;
    }
    index = 0;
    while ((udev->dev = udev->devs[index++]) != NULL) {
      ret = libusb_get_device_descriptor(udev->dev, &udev->desc);
      if ( ret < 0 ) {
        printf("Failed to get device descriptor -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      ret = libusb_open(udev->dev, &udev->handle);
      if ( ret < 0 ) {
        printf("Error opening device -- exit\n");
        libusb_free_device_list(udev->devs,1);
        goto error_exit;
      }

      if( (vid == udev->desc.idVendor) && (pid == udev->desc.idProduct) ) {
        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iManufacturer, (unsigned char*) udev->manufacturer, sizeof(udev->manufacturer));
        if ( ret < 0 ) {
          printf("Error get Manufacturer string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        ret = fby35_common_get_bmc_location(&bmc_location);
        if (ret < 0) {
          syslog(LOG_ERR, "%s() Cannot get the location of BMC", __func__);
          goto error_exit;
        }

        ret = libusb_get_port_numbers(udev->dev, udev->path, sizeof(udev->path));
        if (ret < 0) {
          printf("Error get port number\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        if ( bmc_location == BB_BMC ) {
          if ( udev->path[1] != slot_id) {
            continue;
          }
        }
        printf("%04x:%04x (bus %d, device %d)",udev->desc.idVendor, udev->desc.idProduct, libusb_get_bus_number(udev->dev), libusb_get_device_address(udev->dev));
        printf(" path: %d", udev->path[0]);
        for (index = 1; index < ret; index++) {
          printf(".%d", udev->path[index]);
        }
        printf("\n");

        ret = libusb_get_string_descriptor_ascii(udev->handle, udev->desc.iProduct, (unsigned char*) udev->product, sizeof(udev->product));
        if ( ret < 0 ) {
          printf("Error get Product string descriptor -- exit\n");
          libusb_free_device_list(udev->devs,1);
          goto error_exit;
        }

        printf("Manufactured : %s\n",udev->manufacturer);
        printf("Product : %s\n",udev->product);
        printf("----------------------------------------\n");
        printf("Device Descriptors:\n");
        printf("Vendor ID : %x\n",udev->desc.idVendor);
        printf("Product ID : %x\n",udev->desc.idProduct);
        printf("Serial Number : %x\n",udev->desc.iSerialNumber);
        printf("Size of Device Descriptor : %d\n",udev->desc.bLength);
        printf("Type of Descriptor : %d\n",udev->desc.bDescriptorType);
        printf("USB Specification Release Number : %d\n",udev->desc.bcdUSB);
        printf("Device Release Number : %d\n",udev->desc.bcdDevice);
        printf("Device Class : %d\n",udev->desc.bDeviceClass);
        printf("Device Sub-Class : %d\n",udev->desc.bDeviceSubClass);
        printf("Device Protocol : %d\n",udev->desc.bDeviceProtocol);
        printf("Max. Packet Size : %d\n",udev->desc.bMaxPacketSize0);
        printf("No. of Configuraions : %d\n",udev->desc.bNumConfigurations);

        found = 1;
        break;
      }
    }

    if ( found != 1) {
      sleep(3);
    } else {
      break;
    }
  } while ((--recheck) > 0);


  if ( found == 0 ) {
    printf("Device NOT found -- exit\n");
    libusb_free_device_list(udev->devs,1);
    ret = -1;
    goto error_exit;
  }

  ret = libusb_get_configuration(udev->handle, &udev->config);
  if ( ret != 0 ) {
    printf("Error in libusb_get_configuration -- exit\n");
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }

  printf("Configured value : %d\n", udev->config);
  if ( udev->config != 1 ) {
    libusb_set_configuration(udev->handle, 1);
    if ( ret != 0 ) {
      printf("Error in libusb_set_configuration -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
    printf("Device is in configured state!\n");
  }

  libusb_free_device_list(udev->devs, 1);

  if(libusb_kernel_driver_active(udev->handle, udev->ci) == 1) {
    printf("Kernel Driver Active\n");
    if(libusb_detach_kernel_driver(udev->handle, udev->ci) == 0) {
      printf("Kernel Driver Detached!");
    } else {
      printf("Couldn't detach kernel driver -- exit\n");
      libusb_free_device_list(udev->devs,1);
      goto error_exit;
    }
  }

  ret = libusb_claim_interface(udev->handle, udev->ci);
  if ( ret < 0 ) {
    printf("Couldn't claim interface -- exit. err:%s\n", libusb_error_name(ret));
    libusb_free_device_list(udev->devs,1);
    goto error_exit;
  }
  printf("Claimed Interface: %d, EP addr: 0x%02X\n", udev->ci, udev->epaddr);

  active_config(udev->dev, udev->handle);
  return 0;
error_exit:
  return -1;
}

int
bic_update_cpld_lattice_usb(uint8_t slot_id, uint8_t intf, const char *image, usb_dev* udev) {
  int ret = 0;
  uint32_t fsize = 0;
  uint32_t record_offset = 0;
  CPLDInfo dev_info = {0};
  FILE *fp = NULL;
  uint8_t addr = 0xff;
  uint8_t buf[16] = {0};
  uint8_t transfer_buf[32] = {0};
  uint32_t offset = 0;
  const uint16_t read_count = 16;
  int retry = 0;
  int retries = 3;
  uint8_t data[USB_PKT_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  char fpath[128];

  strcpy(fpath, image);
  fp = fopen(image, "r");
  if ( fp == NULL ) {
    ret = -1;
    goto exit;
  }

  memset(&dev_info, 0, sizeof(dev_info));
  ret = LCMXO2Family_JED_File_Parser(fp, &dev_info);
  if ( ret < 0 ) {
    goto exit;
  }

  fsize = dev_info.CF_Line / 20;

  do {
    // step 1 - detect CPLD
    ret = detect_cpld_dev(slot_id, intf);
    if ( ret < 0 ) {
      printf("Device is not found!\n");
      goto exit;
    }

    //step 1.5 - provide the addr
    addr = CPLD;

    //step 2 - enter transparent mode and check the status
    ret = enter_transparent_mode(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't enter transparent mode!\n");
      goto exit;
    }

    //step 3 - erase the CF and check the status
    ret = erase_flash(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't erase the flash!\n");
      goto exit;
    }

    //step 4 - Transmit Reset Configuration Address
    ret = reset_config_addr(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash!\n");
      goto exit;
    }

    int transferlen = 0;
    int transferred = 0;
    int data_idx, idx, i;
    printf("[Update CPLD via USB]\n");
    while (1) {
      memset(data, 0xFF, sizeof(data));

      for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
        memcpy(buf, (uint8_t *)&dev_info.CF[data_idx], read_count);
        for (idx = 0; idx <= 12; idx+=4) {
            transfer_buf[idx + 0] = buf[idx + 3];
            transfer_buf[idx + 1] = buf[idx + 2];
            transfer_buf[idx + 2] = buf[idx + 1];
            transfer_buf[idx + 3] = buf[idx + 0];
        }

        memcpy(&data[7], (uint8_t *)&transfer_buf, read_count);
        transferlen = read_count + 7;

        data[1] = offset & 0xFF;
        data[2] = (offset >> 8) & 0xFF;
        data[3] = (offset >> 16) & 0xFF;
        data[4] = (offset >> 24) & 0xFF;
        data[5] = read_count & 0xFF;
        data[6] = (read_count >> 8) & 0xFF;

resend:
        ret = libusb_bulk_transfer(udev->handle, udev->epaddr, data, transferlen, &transferred, 3000);
        msleep(1);
        if(((ret != 0) || (transferlen != transferred))) {
          printf("Error in transferring data! err = %d and transferred = %d(expected data length 64)\n",ret ,transferred);
          printf("Retry since  %s\n", libusb_error_name(ret));
          retries--;
          if (!retries) {
            ret = -1;
            goto exit;
          }
          msleep(100);
          goto resend;
        }

        offset += read_count;
        if ( (record_offset + fsize) <= i ) {
          _set_fw_update_ongoing(slot_id, 60);
          printf("updated cpld: %d %%\n", (i/fsize)*5);
          record_offset += fsize;
        }
      }

      if (i == dev_info.CF_Line) {
        break;
      }
    }

    //step 6 - program user code
    ret = program_user_code(slot_id, intf, addr, (uint8_t *)&dev_info.Version);
    if ( ret < 0 ) {
      printf("Couldn't program the usercode!\n");
      goto exit;
    }

    //step 7 - verify the CF data
    //step 7.1 Transmit Reset Configuration Address again
    ret = reset_config_addr(slot_id, intf, addr);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash again!\n");
      goto exit;
    }

    record_offset = 0;
    //step 7.2 Transmit Read Command with Number of Pages
    for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
      ret = verify_cf(slot_id, intf, addr, (uint8_t *)&dev_info.CF[data_idx]);
      if ( ret < 0 ) {
        printf("Read cf data but ret < 0. exit\n");
        goto exit;//break;
      }

      if ( (record_offset + fsize) <= i ) {
        printf("verify cpld: %d %%\n", (i/fsize)*5);
        record_offset += fsize;
      }
    }

    if (i == dev_info.CF_Line) {
      break;
    }
  } while ((++retry) < RETRY_TIME);

  if (retry == RETRY_TIME) {
    goto exit;
  }

  //step 8 - program done
  ret = program_done(slot_id, intf, addr);
  if ( ret < 0 ) {
    printf("Couldn't finish the program!\n");
    goto exit;
  }

exit:
  if ( fp != NULL ) fclose(fp);
  if ( dev_info.CF != NULL ) free(dev_info.CF);

  return ret;
}

int
bic_close_exp_usb_dev(usb_dev* udev) {
  if (libusb_release_interface(udev->handle, udev->ci) < 0) {
    printf("Couldn't release the interface 0x%X\n", udev->ci);
  }

  if (udev->handle != NULL )
    libusb_close(udev->handle);
  libusb_exit(NULL);

  return 0;
}

int
update_bic_cpld_lattice_usb(uint8_t slot_id, char *image, uint8_t intf, uint8_t force) {
  struct timeval start, end;
  int ret = 0;
  usb_dev   bic_udev;
  usb_dev*  udev = &bic_udev;

  udev->ci = 1;
  udev->epaddr = 0x1;

  // init usb device
  ret = bic_init_exp_usb_dev(slot_id, intf, udev);
  if (ret < 0) {
    goto error_exit;
  }

  printf("Input: %s, USB timeout: 3000ms\n", image);
  gettimeofday(&start, NULL);

  // sending file
  ret = bic_update_cpld_lattice_usb(slot_id, intf, image, udev);
  if (ret < 0) {
    goto error_exit;
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));
  ret = 0;

error_exit:
  // close usb device
  bic_close_exp_usb_dev(udev);

  return ret;
}
