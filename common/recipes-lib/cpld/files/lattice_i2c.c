#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include <openbmc/obmc-i2c.h>
#include "cpld.h"
#include "lattice.h"
#include "lattice_i2c.h"

#define CHECK_STATUS_RETRY 100

enum {
  CFG0 = 0,
  CFG1,
  UFM0,
  UFM1,
  UFM2,
};

typedef enum {
  XO2_XO3 = 0,
  NX,
} cpld_type;



int g_i2c_fd = 0;
uint8_t g_i2c_addr = 0;

static int
read_cpld_busy_flag() {
  //support XO2, XO3, NX
  int ret = 0;
  uint8_t tbuf[4]= {LSC_CHECK_BUSY, 0x0, 0x0, 0x0};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 1;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send read_cpld_busy_flag cmd\n");
    return ret;
  }
  ret = (rbuf[0] & 0x80) >> 7;

  return (ret == 0x0) ? 0 : -1;
}

static int
read_cpld_status0_flag() {
  //support XO2, XO3, NX
  int ret = 0;
  uint8_t tbuf[4]= {LSC_READ_STATUS, 0x0, 0x0, 0x0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 4;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't read_cpld_status0_flag cmd\n");
    return ret;
  }
  ret = (rbuf[2] >> 4) & 0x3; //bit12,13

  return (ret == 0x0) ? 0 : -1;
}

static int
read_cpld_status1_flag() {
  int ret = 0;
  uint8_t tbuf[4]= {LSC_READ_STATUS1, 0x0, 0x0, 0x0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 4;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't read_cpld_status1_flag cmd\n");
    return ret;
  }

  // printf("status1: %02X %02X %02X %02X \n", rbuf[0], rbuf[1], rbuf[2], rbuf[3]);
  return 0;
}

static int
enter_transparent_mode() {
  int ret = 0;
  int retry = CHECK_STATUS_RETRY;
  uint8_t tbuf[3]= {ISC_ENABLE_X, 0x08, 0x0};
  uint8_t rbuf[2] = {0};
  uint8_t tlen = 3;
  uint8_t rlen = 0;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send enter_transparent_mode cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(50*1000);
    else break;
  } while( retry-- );

  return ret;
}

static int
erase_flash(cpld_type type, CPLDInfo *dev_info) {
  int ret = 0;
  int retry = CHECK_STATUS_RETRY;
  uint8_t tbuf[4]= {ISC_ERASE, 0x0, 0x0, 0x0};
  uint8_t rbuf[2] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  if (type == XO2_XO3) {
    if ( dev_info->CF_Line ) {
      tbuf[1] |= 1 << 2;  //bit18
    }
    if ( dev_info->UFM_Line ) {
      tbuf[1] |= 1 << 3;  //bit19
    }
  } else if (type == NX) {
    //NX
    if ( dev_info->CF_Line ) {
      tbuf[2] |= 1 << 0;  //bit8
    }
    if ( dev_info->UFM_Line ) {
      tbuf[2] |= 1 << 2;  //bit10
    }
  } else {
    return -1;
  }

  printf("erase command : %02X %02X %02X %02X\n",tbuf[0],tbuf[1],tbuf[2],tbuf[3]);

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send erase flash cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(50*1000);
    else break;
  } while( retry-- );
  if ( ret ) {
    printf("check busy flag fail, after retry:%d \n", CHECK_STATUS_RETRY);
    return ret;
  }

  retry = CHECK_STATUS_RETRY;
  do {
    ret = read_cpld_status0_flag();
    if ( ret < 0 ) usleep(50*1000);
    else break;
  } while( retry-- );
  if ( ret ) {
    printf("check status0 flag fail, after retry:%d \n", CHECK_STATUS_RETRY);
  }

  return ret;
}

static int
reset_addr(cpld_type type, uint8_t sector) {
  int ret = 0;
  uint8_t tbuf[4] = {0};
  uint8_t rbuf[2] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  if (type == XO2_XO3) {
    switch (sector) {
      case CFG0:
        tbuf[0] = LSC_INIT_ADDRESS;
        break;
      case UFM0:
        tbuf[0] = LSC_INIT_ADDR_UFM;
        break;
    }
  } else if (type == NX){
    switch (sector) {
      case CFG0:
        tbuf[0] = LSC_INIT_ADDRESS;
        tbuf[2] |= 1 << 0;  //bit8
        break;
      case UFM0:
        tbuf[0] = LSC_INIT_ADDRESS;
        tbuf[2] |= 1 << 6;  //bit14
        break;
    }
  } else {
    return -1;
  }


  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send reset memory addr cmd\n");
  }

  return ret;
}

static uint8_t
bit_swap(uint8_t input) {
  uint8_t output = 0;

  for (int bit = 0; bit < 8; bit++) {
    output |= ( ((input & (1<<bit)) >> bit) << (7-bit) );
  }

  return output;
}

static int
program_page(cpld_type type, uint8_t *data) {
  int ret = 0;
  int retry = CHECK_STATUS_RETRY;
  uint8_t tbuf[20]= {LSC_PROG_INCR_NV, 0x0, 0x0, 0x01};
  uint8_t rbuf[16] = {0};
  uint8_t tlen = 20;
  uint8_t rlen = 0;

  for (int index = 0; index < 16; index++) {
    tbuf[index + 4] = bit_swap(data[index]);
  }

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send program_page cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(10*1000);
    else break;
  } while( retry--);

  return ret;
}

static int
program_sector(cpld_type type, CPLDInfo *dev_info, uint8_t sector) {
  uint32_t fsize = dev_info->CF_Line / 20;
  uint32_t record_offset = 0;
  int i , data_idx;
  int ret = 0;

  unsigned int line = 0;
  unsigned int *data;

  switch (sector) {
    case CFG0:
      line = dev_info->CF_Line;
      data = dev_info->CF;
      break;
    case UFM0:
      line = dev_info->UFM_Line;
      data = dev_info->UFM;
      break;
  }

  for (i = 0, data_idx = 0; i < line; i++, data_idx+=4) {
    ret = program_page(type, (uint8_t *)&data[data_idx]);
    if ( ret < 0 ) {
      printf("send data but ret < 0 at line:%d. exit!\n",i);
      return ret;
    }
    if ( (record_offset + fsize) <= i ) {
      printf("\rupdated cpld: %d %%", (int) (i/fsize)*5);
      fflush(stdout);
      record_offset += fsize;
    }
  }
  printf("\n");

  return ret;
}

static int
cpld_program_i2c(cpld_type type, CPLDInfo *dev_info) {
  int ret = 0;

  if ( dev_info->CF_Line ) {
    ret = reset_addr(type, CFG0);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash!\n");
      return ret;
    }
    ret = program_sector(type, dev_info, CFG0);
  }

  if ( dev_info->UFM_Line ) {
    ret = reset_addr(type, UFM0);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash!\n");
      return ret;
    }
    ret = program_sector(type, dev_info, UFM0);
  }

  return ret;
}

static int
verify_page(cpld_type type, uint8_t *data) {
  int ret = 0;
  int retry = CHECK_STATUS_RETRY;
  uint8_t tbuf[4]= {LSC_READ_INCR_NV, 0x00, 0x00, 0x01};
  uint8_t tlen = 4;
  uint8_t rbuf[16] = {0};
  uint8_t cmp_data[16] = {0};
  uint8_t rlen = 16;

  for (int index = 0; index < 16; index++) {
    cmp_data[index] = bit_swap(data[index]);
  }

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send program_page cmd\n");
    return ret;
  }

  if ( 0 != memcmp(rbuf, cmp_data, rlen)) {
    printf("Data verify fail\n");
    return -1;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(10*1000);
    else break;
  } while( retry--);

  return ret;
}

static int
verify_sector(cpld_type type, CPLDInfo *dev_info, uint8_t sector) {
  unsigned int line = 0;
  unsigned int *data;
  int ret = 0;
  int record_offset = 0;
  uint32_t fsize = 0;

  switch (sector) {
    case CFG0:
      line = dev_info->CF_Line;
      data = dev_info->CF;
      break;
  }

  fsize = line / 20;

  for (int i = 0, data_idx = 0; i < line; i++, data_idx+=4) {
    ret = verify_page(type, (uint8_t *)&data[data_idx]);
    if ( ret < 0 ) {
      printf("Read cf data but ret < 0. exit\n");
      return ret;
    }

    if ( (record_offset + fsize) <= i ) {
      printf("\rverify cpld: %d %%", (int) (i/fsize)*5);
      fflush(stdout);
      record_offset += fsize;
    }
  }
  printf("\n");

  return ret;
}

static int
cpld_verify_i2c(cpld_type type, CPLDInfo *dev_info) {
  int ret = 0;

  if ( dev_info->CF_Line ) {
    ret = reset_addr(type, CFG0);
    if ( ret < 0 ) {
      printf("Couldn't reset the flash!\n");
      return ret;
    }

    ret = verify_sector(type, dev_info, CFG0);
    if ( ret < 0 ) {
      printf("verify_sector fail!\n");
      return ret;
    }
  }

  // if ( dev_info->UFM_Line ) {
  //
  // }

  return ret;
}

static int
program_user_code(cpld_type type, uint8_t *user_data) {
  int ret = 0;
  int retry = CHECK_STATUS_RETRY;
  uint8_t tbuf[8]= {ISC_PROGRAM_USERCODE, 0x0, 0x0, 0x0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 8;
  uint8_t rlen = 0;

  ret = reset_addr(type, CFG0);
  if ( ret < 0 ) {
    printf("Couldn't reset the flash!\n");
    return ret;
  }

  tbuf[4] = user_data[3];
  tbuf[5] = user_data[2];
  tbuf[6] = user_data[1];
  tbuf[7] = user_data[0];

  // printf("user code txbuf:\n");
  // for(int i=0;i<8;i++) {
  //   printf("%02X ",tbuf[i]);
  // }
  // printf("\n");

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send program_user_code cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(10*1000);
    else break;
  } while( retry-- );

  ret = reset_addr(type, CFG0);
  if ( ret < 0 ) {
    printf("Couldn't reset the flash!\n");
    return ret;
  }

  //read user code for check
  tbuf[0] = USERCODE;
  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, 4, rbuf, 4);
  if ( ret < 0 ) {
    printf("Couldn't send program_user_code cmd\n");
    return ret;
  }
  printf("read user code:\n");
  for(int i=0;i<4;i++) {
    printf("%02X ",rbuf[i]);
  }
  printf("\n");

  ret = memcmp(rbuf, &tbuf[4], 4);

  return ret;
}

static int
program_done(cpld_type type) {
  int ret;
  uint8_t tbuf[4]= {ISC_PROGRAM_DONE, 0x0, 0x0, 0x0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;
  int retry = CHECK_STATUS_RETRY;

  ret = reset_addr(type, CFG0);
  if ( ret < 0 ) {
    printf("Couldn't reset the flash!\n");
  }

  // printf("tbuf: ");
  // for(int i=0; i<tlen; i++) {
  //   printf("%02X ",tbuf[i]);
  // }
  // printf("\n");

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send program_done cmd\n");
    return ret;
  }

  do {
    ret = read_cpld_busy_flag();
    if ( ret < 0 ) usleep(10*1000);
    else break;
  } while( retry-- );

  read_cpld_status1_flag();

  return ret;
}


static int exit_program_mode() {
  printf("Sending exit program mode cmd \n");
  int ret;
  uint8_t tbuf[3]= {ISC_DISABLE, 0x0, 0x0};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 3;
  uint8_t rlen = 0;

  // printf("tbuf: ");
  // for(int i=0; i<tlen; i++) {
  //   printf("%02X ",tbuf[i]);
  // }
  // printf("\n");

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send program done cmd\n");
    return ret;
  }

  return ret;
}

static int bypass_instruction() {
  printf("Sending bypass instruction cmd \n");
  int ret;
  uint8_t tbuf[4]= {0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 0;

  // printf("tbuf: ");
  // for(int i=0; i<tlen; i++) {
  //   printf("%02X ",tbuf[i]);
  // }
  // printf("\n");

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if ( ret < 0 ) {
    printf("Couldn't send bypass instruction cmd\n");
    return ret;
  }

  return ret;
}


int common_cpld_Get_Ver_i2c(unsigned int *ver)
{
  //support XO2, XO3, NX
  int ret = 0;
  uint8_t tbuf[4] = {USERCODE, 0x00, 0x00, 0x00}; //UserCode
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  unsigned int version = 0;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d \n", __func__, tlen);
    return -1;
  } else {
    version = rbuf[0] << 0;
    version |= rbuf[1] << 8;
    version |= rbuf[2] << 16;
    version |= rbuf[3] << 24;
    *ver = version;
  }

  return 0;
}

int common_cpld_Get_id_i2c(unsigned int *dev_id)
{
  // support XO2, XO3, NXFamily
  int ret = 0;
  uint8_t tbuf[4] = {IDCODE_PUB, 0x00, 0x00, 0x00}; //device id
  uint8_t rbuf[4] = {0};
  uint8_t tlen = 4;
  uint8_t rlen = 4;
  unsigned int device_id = 0;

  ret = i2c_rdwr_msg_transfer(g_i2c_fd, g_i2c_addr << 1, tbuf, tlen, rbuf, rlen);
  if (ret < 0) {
    printf("%s() Failed to do i2c_rdwr_msg_transfer, tlen=%d \n", __func__, tlen);
    return -1;
  } else {
    device_id = rbuf[0] << 24;
    device_id |= rbuf[1] << 16;
    device_id |= rbuf[2] << 8;
    device_id |= rbuf[3] << 0;
    *dev_id = device_id;
  }

  return 0;
}

int common_cpld_Check_ID()
{
  // support XO2, XO3, NXFamily
  int ret = 0;
  int i = 0;
  int dev_cnts=0;
  unsigned int device_id = 0;
  struct cpld_dev_info* dev_list;


  ret = common_cpld_Get_id_i2c(&device_id);
  if (ret == 0) {
    printf("Device ID : %08X \n",device_id);
  } else {
    return ret;
  }


  dev_cnts += get_lattice_dev_list(&dev_list);

  for (i = 0; i < dev_cnts; i++) {
    if (device_id == dev_list[i].dev_id) {
      break;
    }
  }

  if (i == dev_cnts) {
    printf("device ID is not in lattice_dev_list\n");
    return -1;
  } else {
    return 0;
  }
}

int XO2XO3Family_cpld_update_i2c(FILE *jed_fd, char* key, char is_signed)
{
  CPLDInfo dev_info = {0};
  int cf_size = 0;
  int ufm_size = 0;
  int ret = 0;

  ret = common_cpld_Check_ID();
  if ( ret < 0 )
  {
    printf("[%s] Unknown Device ID!\n", __func__);
    goto error_exit;
  }

  //set file pointer to the beginning
  fseek(jed_fd, 0, SEEK_SET);

  //get update data size
  ret = LCMXO2Family_Get_Update_Data_Size(jed_fd, &cf_size, &ufm_size);
  if ( ret < 0 )
  {
    printf("[%s] Update Data Size Error!\n", __func__);
    goto error_exit;
  }

  //set file pointer to the beginning
  fseek(jed_fd, 0, SEEK_SET);

  //parse info from JED file and calculate checksum
  ret = LCMXO2Family_JED_File_Parser(jed_fd, &dev_info, cf_size, ufm_size);
  if ( ret < 0 )
  {
    printf("[%s] JED file CheckSum Error!\n", __func__);
    goto error_exit;
  }

  ret = enter_transparent_mode();
  if ( ret < 0 ) {
    printf("Couldn't enter transparent mode!\n");
    goto error_exit;
  }

  ret = erase_flash(XO2_XO3, &dev_info);
  if ( ret < 0 ) {
    printf("Couldn't erase the flash!\n");
    goto error_exit;
  }

  ret = cpld_program_i2c(XO2_XO3, &dev_info);
  if ( ret < 0 )
  {
    printf("Program failed!\n");
    goto error_exit;
  }

  ret = program_user_code(XO2_XO3, (uint8_t *)&dev_info.Version);
  if ( ret < 0 ) {
    printf("Couldn't program the usercode!\n");
    goto error_exit;
  }

  ret = cpld_verify_i2c(XO2_XO3, &dev_info);
  if ( ret < 0 )
  {
    printf("verify failed!\n");
    goto error_exit;
  }

  ret = program_done(XO2_XO3);
  if ( ret < 0 ) {
    printf("Couldn't finish the program!\n");
    goto error_exit;
  }

  ret = exit_program_mode();
  if ( ret < 0 ) {
    printf("Couldn't exit the program mode!\n");
    goto error_exit;
  }

  ret = bypass_instruction();
  if ( ret < 0 ) {
    printf("Couldn't sent bypass instruction!\n");
    goto error_exit;
  }

error_exit:
  if ( NULL != dev_info.CF )
  {
    free(dev_info.CF);
  }

  if ( NULL != dev_info.UFM )
  {
    free(dev_info.UFM);
  }

  return ret;
}

int NXFamily_cpld_update_i2c(FILE *jed_fd, char* key, char is_signed)
{
  printf("NXFamily_cpld_update_i2c \n");
  CPLDInfo dev_info = {0};
  int cf_size = 0;
  int ufm_size = 0;
  int ret = 0;

  ret = common_cpld_Check_ID();
  if ( ret < 0 )
  {
    printf("[%s] Unknown Device ID!\n", __func__);
    goto error_exit;
  }

  //set file pointer to the beginning
  fseek(jed_fd, 0, SEEK_SET);

  //get update data size
  ret = NX_Get_Update_Data_Size(jed_fd, &cf_size, &ufm_size); //todo : NX UFM key word is different
  if ( ret < 0 )
  {
    printf("[%s] Update Data Size Error!\n", __func__);
    goto error_exit;
  }

  //set file pointer to the beginning
  fseek(jed_fd, 0, SEEK_SET);

  //parse info from JED file and calculate checksum
  ret = NX_JED_File_Parser(jed_fd, &dev_info, cf_size, ufm_size); //todo : NX UFM key word is different
  if ( ret < 0 )
  {
    printf("[%s] JED file CheckSum Error!\n", __func__);
    goto error_exit;
  }

  ret = enter_transparent_mode();
  if ( ret < 0 ) {
    printf("Couldn't enter transparent mode!\n");
    goto error_exit;
  }

  ret = erase_flash(NX, &dev_info);
  if ( ret < 0 ) {
    printf("Couldn't erase the flash!\n");
    goto error_exit;
  }

  ret = cpld_program_i2c(NX, &dev_info);
  if ( ret < 0 )
  {
    printf("Program failed!\n");
    goto error_exit;
  }

  ret = program_user_code(NX, (uint8_t *)&dev_info.Version);
  if ( ret < 0 ) {
    printf("Couldn't program the usercode!\n");
    goto error_exit;
  }

  ret = cpld_verify_i2c(NX, &dev_info);
  if ( ret < 0 )
  {
    printf("verify failed!\n");
    goto error_exit;
  }

  ret = program_done(NX);
  if ( ret < 0 ) {
    printf("Couldn't finish the program!\n");
    goto error_exit;
  }

  ret = exit_program_mode();
  if ( ret < 0 ) {
    printf("Couldn't exit the program mode!\n");
    goto error_exit;
  }

  ret = bypass_instruction();
  if ( ret < 0 ) {
    printf("Couldn't sent bypass instruction!\n");
    goto error_exit;
  }

error_exit:
  if ( NULL != dev_info.CF )
  {
    free(dev_info.CF);
  }

  if ( NULL != dev_info.UFM )
  {
    free(dev_info.UFM);
  }

  return ret;
}

int cpld_dev_open_i2c(void *_attr) {
  i2c_attr_t *attr = (i2c_attr_t *)_attr;
  int i2c_fd = 0;

  syslog(LOG_INFO, "%s, bus_id = %02X, slv_addr = %02X \n", __FUNCTION__, attr->bus_id, attr->slv_addr);
  i2c_fd = i2c_cdev_slave_open(attr->bus_id, attr->slv_addr, I2C_SLAVE_FORCE_CLAIM);
  if ( i2c_fd < 0 ) {
    printf("%s() Failed to open bus %u addr %u . Err: %s", __func__, attr->bus_id, attr->slv_addr, strerror(errno));
    return -1;
  }
  g_i2c_fd = i2c_fd;
  g_i2c_addr = attr->slv_addr;
  return 0;
}

int cpld_dev_close_i2c(void) {
  close(g_i2c_fd);
  g_i2c_fd = 0;
  g_i2c_addr = 0;
  return 0;
}
