#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "cpld.h"
#include "altera.h"
#include <openbmc/pal.h>

//#define _DEBUGMSG_

#ifdef _DEBUGMSG_
#define DEBUGMSG(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
#define DEBUGMSG(args...)
#endif

// status register
#define BUSY_IDLE         0x00
#define BUSY_ERASE        0x01
#define BUSY_WRITE        0x02
#define BUSY_READ         0x03
#define READ_SUCCESS      0x04
#define WRITE_SUCCESS     0x08
#define ERASE_SUCCESS     0x10
#define STATUS_BIT_MASK   0x1F

#define SIGNED_FULL_SIZE                 (32)
#define SIGNED_HEAD_SIZE                 (5)
#define SIGNED_DATA_SIZE                 SIGNED_FULL_SIZE - SIGNED_HEAD_SIZE


enum {
  PROTECT_SEC_ID_5 = 0x1<<27,
  PROTECT_SEC_ID_4 = 0x1<<26,
  PROTECT_SEC_ID_3 = 0x1<<25,
  PROTECT_SEC_ID_2 = 0x1<<24,
  PROTECT_SEC_ID_1 = 0x1<<23,
};

enum {
	CFM_IMAGE_NONE = 0,
	CFM_IMAGE_1,
	CFM_IMAGE_2,
	CFM_IMAGE_1_M04,
};

// According to QSYS setting in FPGA project
static int g_i2c_file = 0;
static uint8_t g_i2c_bridge_addr = 0;
static char g_i2c_file_dp[64];
static unsigned long g_i2c_funcs = 0;


// on-chip Flash IP
static uint32_t g_flash_csr_base = 0;
static uint32_t g_flash_csr_status_reg = 0;
static uint32_t g_flash_csr_ctrl_reg = 0;
static uint32_t g_flash_data_reg = 0;

// Dual-boot IP
static uint32_t g_dual_boot_base = 0;

// CFM Info
static uint32_t g_cfm_start_addr = 0;
static uint32_t g_cfm_end_addr = 0;
static uint32_t g_cfm_image_type = 0;

int set_i2c_register(int file, uint8_t addr, int reg, int value)
{
  uint8_t outbuf[8];

  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    /* ERROR HANDLING; you can check errno to see what went wrong */
    return -1;
  }

  // write register  4 bytes
  outbuf[0] = (reg >> 24 ) & 0xFF;
  outbuf[1] = (reg >> 16 ) & 0xFF;
  outbuf[2] = (reg >> 8 )  & 0xFF;
  outbuf[3] = (reg >> 0 )  & 0xFF;
  outbuf[4] = (value >> 0 ) & 0xFF;
  outbuf[5] = (value >> 8 ) & 0xFF;
  outbuf[6] = (value >> 16 )  & 0xFF;
  outbuf[7] = (value >> 24 )  & 0xFF;

  return write(file, outbuf, 8);
}

int get_i2c_register(int file, uint8_t addr, uint32_t reg, uint32_t *val)
{
  int ret = 0;
  uint8_t outbuf[4], inbuf[4];
  uint32_t readval;
  struct i2c_msg messages[] = {
    { addr, 0, sizeof(outbuf), outbuf },
    { addr, I2C_M_RD, sizeof(inbuf), inbuf },
  };
  struct i2c_rdwr_ioctl_data ioctl_data = { messages, 2 };

  if (ioctl(file, I2C_SLAVE, addr) < 0) {
    /* ERROR HANDLING; you can check errno to see what went wrong */
    return -1;
  }

  // write register  4 bytes
  outbuf[0] = (reg >> 24 ) & 0xFF;
  outbuf[1] = (reg >> 16 ) & 0xFF;
  outbuf[2] = (reg >> 8 )  & 0xFF;
  outbuf[3] = (reg >> 0 )  & 0xFF;

  if (g_i2c_funcs & I2C_FUNC_I2C) {
    ret = ioctl(file, I2C_RDWR, &ioctl_data);
    if (ret == 2) {
      ret = sizeof(inbuf);  // update read length
    } else {
      ret = 0;
    }
  } else {
    ret = write(file, outbuf, 4);
    DEBUGMSG(" write() ret = %d\r\n", ret);

    // dummy read 4 byte
    ret = read(file, inbuf, 4);
  }

  readval =  (inbuf[0] << 0);
  readval |= (inbuf[1] << 8);
  readval |= (inbuf[2] << 16);
  readval |= (inbuf[3] << 24);

  *val = readval;
  DEBUGMSG(" val = %x len=%d\r\n", *val, ret);

  return ret;
}

void max10_update_init(int i2c_file)
{
  g_i2c_file = i2c_file;

  // check i2c functionality
  if (ioctl(g_i2c_file, I2C_FUNCS, &g_i2c_funcs) < 0) {
     /* ERROR HANDLING; you can check errno to see what went wrong */
     syslog(LOG_WARNING, "ioctl(file, I2C_FUNCS) fail.");
     g_i2c_funcs = 0;
  }

  // Set timeout /* set timeout in units of 10 ms */
  if (ioctl(g_i2c_file, I2C_TIMEOUT, 1000) < 0) {
     /* ERROR HANDLING; you can check errno to see what went wrong */
     syslog(LOG_WARNING, "ioctl(file, I2C_TIMEOUT) fail.");
  }

  // set retry time
  if (ioctl(g_i2c_file, I2C_RETRIES, 100) < 0) {
     /* ERROR HANDLING; you can check errno to see what went wrong */
     syslog(LOG_WARNING, "ioctl(file, I2C_RETRIES) fail.");
  }
}

void max10_done()
{
  close(g_i2c_file);
  g_i2c_file = 0;
  g_i2c_funcs = 0;
}

int max10_reg_write(int address, int data)
{
  int ret ;

  if (g_i2c_file == 0) {
    printf("\n i2c device not specified. \n");
    return -1;
  } else {
    ret = set_i2c_register(g_i2c_file, g_i2c_bridge_addr, address, data);
    if (ret != 8) {
      syslog(LOG_WARNING, "set_i2c_register() ERROR. ret = %d.", ret);
    }
  }

  usleep(100);
  return ret;
}

int max10_reg_read(uint32_t address)
{
  uint32_t data;
  int ret;

  if (g_i2c_file == 0) {
    printf("\n i2c device not specified. \n");
    return -1;
  } else {
    ret = get_i2c_register(g_i2c_file, g_i2c_bridge_addr, address, &data);
    if (ret != 4) {
      syslog(LOG_WARNING, "get_i2c_register() ERROR. ret = %d.", ret);
    }
  }

  //usleep(1000);
  return data;
}

int max10_protect_sectors(void)
{
  int ret = 0;
  int value;

  DEBUGMSG("Enter %s\r\n", __func__);

  value = max10_reg_read(g_flash_csr_ctrl_reg);
  value |= (0x1F<<5);
  ret = max10_reg_write(g_flash_csr_ctrl_reg, value);

  DEBUGMSG("Exit %s\r\n", __func__);
  return ret;
}

int max10_unprotect_sector(int sector_id)
{
  int ret = 0;
  int value;
  int val_before, val_after;

  printf("Max10 Unprotect Sector %d. \r\n", sector_id);

  value = max10_reg_read(g_flash_csr_ctrl_reg);
  val_before = value;
  DEBUGMSG("\t read ctrl reg = 0x%08X. \r\n", val_before);

  switch(sector_id)
  {
    case 5:
      value = value & (~PROTECT_SEC_ID_5);  // set to zero
      break;
    case 4:
      value = value & (~PROTECT_SEC_ID_4);
      break;
    case 3:
      value = value & (~PROTECT_SEC_ID_3);
      break;
    case 2:
      value = value & (~PROTECT_SEC_ID_2);
      break;
    case 1:
      value = value & (~PROTECT_SEC_ID_1);
      break;
    case 0:
    default:
      value = 0xFFFFFFFF;
      break;
  }

  DEBUGMSG("\t write ctrl reg = 0x%08X. \r\n", value);
  ret = max10_reg_write(g_flash_csr_ctrl_reg, value);

  val_after = max10_reg_read(g_flash_csr_ctrl_reg);
  DEBUGMSG("\t read ctrl reg = 0x%08X. \r\n", val_after);

  // compare
  value = val_after ^ val_before;
  DEBUGMSG("\t diff = 0x%08X. \r\n", value);

  DEBUGMSG("Exit %s\r\n", __func__);
  return ret;
}

int max10_unprotect_sector_all(void)
{
  int ret = 0;
  int value;
  int val_before, val_after;

  DEBUGMSG("Enter %s\r\n", __func__);

  value = max10_reg_read(g_flash_csr_ctrl_reg);
  val_before = value;
  DEBUGMSG("\t read ctrl reg = 0x%08X. \r\n", val_before);

  value = value & ~(0x1F<<23);  // set to zero
  DEBUGMSG("\t write ctrl reg = 0x%08X. \r\n", value);
  ret = max10_reg_write(g_flash_csr_ctrl_reg, value);

  val_after = max10_reg_read(g_flash_csr_ctrl_reg);
  DEBUGMSG("\t read ctrl reg = 0x%08X. \r\n", val_after);

  // compare
  value = val_after ^ val_before;
  DEBUGMSG("\t diff = 0x%08X. \r\n", value);

  DEBUGMSG("Exit %s\r\n", __func__);
  return ret;
}

int max10_erase_sector(int sector_id)
{
  int ret = 0;
  int value;
  int status;
  int done = 0;
  int cnt = 0;

  //Control register bit 20~22
  enum{
    SECTOR_ID_5  = 0b101 << 20,
    SECTOR_ID_4  = 0b100 << 20,
    SECTOR_ID_3  = 0b011 << 20,
    SECTOR_ID_2  = 0b010 << 20,
    SECTOR_ID_1  = 0b001 << 20,
    Sector_erase_NotSet = 0b111 << 20,
  };

  printf("Max10 Erase Sector %d \r\n", sector_id);

  value = max10_reg_read(g_flash_csr_ctrl_reg);
  DEBUGMSG("\t read ctrl reg = 0x%08X. \r\n", value);

  value &= ~(0x7<<20);  // clear bit 20~22.

  switch(sector_id)
  {
    case 5:
      value |= SECTOR_ID_5;
      break;
    case 4:
      value |= SECTOR_ID_4;
      break;
    case 3:
      value |= SECTOR_ID_3;
      break;
    case 2:
      value |= SECTOR_ID_2;
      break;
    case 1:
      value |= SECTOR_ID_1;
      break;
    case 0:
      value |= Sector_erase_NotSet;
      break;
    default:
      printf("\t WRONG sector id = 0x%08X. \r\n", sector_id);
  }

  max10_reg_write(g_flash_csr_ctrl_reg, value);
  DEBUGMSG("\t write ctrl reg = 0x%08X. \r\n", value);
  cnt = 0;

  //Wait erase finished.
  while(!done) {
    status = max10_reg_read(g_flash_csr_status_reg);
    DEBUGMSG("\t read status reg = 0x%08X. \r\n", status);

    if (cnt < MAX10_RETRY_TIMEOUT) {
      cnt++;
      usleep(10);
    } else {
      printf("Programming Fail Timeout\n");
      return -1;
    }

    if (status & BUSY_ERASE) {
      usleep(500*1000);
      continue;
    }

    if (status & ERASE_SUCCESS) {
      done = 1;
      printf("Erase sector SUCCESS. \r\n");
    } else {
      printf("Erase sector FAIL. \r\n");
      ret = -1;
    }
  }

  return ret;
}

// write 4 byte into on-chip flash.
int max10_flash_write(int address, int data)
{
  int ret = 0;

  ret = max10_reg_write(g_flash_data_reg + address, data);
  return ret;
}

// Read 4 byte from on-chip flash.
int max10_flash_read(int address)
{
  int ret = 0;

  ret = max10_reg_read(g_flash_data_reg + address);
  return ret;
}

int max10_status_read(void)
{
  int ret = 0;

  ret = max10_reg_read(g_flash_csr_status_reg);
  return ret;
}

int max10_is_busy(void)
{
  int value;

  value = max10_reg_read(g_flash_csr_status_reg);
  if (value & (BUSY_ERASE|BUSY_READ|BUSY_WRITE))
    return 1;
  else
    return 0; // timeout
}

int max10_update_rpd(uint8_t* rpd_file, uint8_t image_type, int cfm_start_addr, int cfm_end_addr)
{
  int ret = 0;
  int address;
  int data;
  int receivedHex[4];
  int byte;
  int bContinue = 1;
  int status;
  int offset;
  int cnt;

  printf("OnChip Flash Status = 0x%X. \r\n", max10_status_read());
  printf("image type = 0x%X. \r\n", image_type);

  switch (image_type)
  {
    case CFM_IMAGE_1:
      ret = max10_unprotect_sector(5);
      ret = max10_erase_sector(5);
      break;
    case CFM_IMAGE_2:
      ret = max10_unprotect_sector(3);
      ret = max10_unprotect_sector(4);
      ret = max10_erase_sector(3);
      ret = max10_erase_sector(4);
      break;
    case CFM_IMAGE_1_M04:
      ret = max10_unprotect_sector(4);
      ret = max10_erase_sector(4);
      break;
    case CFM_IMAGE_NONE:
      printf(" WRONG image type = 0x%X. \r\n", image_type);
      return -1;
  }

  if (ret != 0) {
    printf("Erase sector fail. \r\n");
    return -1;
  }

  // Set erase none.
  ret = max10_erase_sector(0);

  // start program
  offset = 0;
  for(address = cfm_start_addr; address < cfm_end_addr; address += 4) {
    /*Get 4 bytes from UART Terminal*/
    receivedHex[0] = rpd_file[offset + 0];
    receivedHex[1] = rpd_file[offset + 1];
    receivedHex[2] = rpd_file[offset + 2];
    receivedHex[3] = rpd_file[offset + 3];

    /*Swap LSB with MSB before write into CFM*/
    for(byte=0; byte<4; byte++)
    {
      receivedHex[byte] = (((receivedHex[byte] & 0xaa)>>1)|((receivedHex[byte] & 0x55)<<1));
      receivedHex[byte] = (((receivedHex[byte] & 0xcc)>>2)|((receivedHex[byte] & 0x33)<<2));
      receivedHex[byte] = (((receivedHex[byte] & 0xf0)>>4)|((receivedHex[byte] & 0x0f)<<4));
    }

    /*Combine 4 bytes to become 1 word before write operation*/
    data = (receivedHex[0]<<24)|(receivedHex[1]<<16)|(receivedHex[2]<<8)|(receivedHex[3]);

    DEBUGMSG("%s write data=%x\n", __func__, data);
    /*Command to write into On-Chip Flash IP*/
    max10_flash_write(address, data);

    // Wait for write operation
    bContinue = 1;
    cnt = 0;

    while( bContinue ) {
      status = max10_status_read();
      status &= STATUS_BIT_MASK;

      if (cnt < MAX10_RETRY_TIMEOUT) {
        cnt++;
        usleep(10);
      } else {
        printf("Programming Fail Timeout\n");
        return -1;
      }

      if( (status & BUSY_WRITE) == BUSY_WRITE ){
        DEBUGMSG("Writing CFM0(0x%X)\n",address);
        continue;
      }

      if( (status & WRITE_SUCCESS) == WRITE_SUCCESS){
        DEBUGMSG("Write to addr: 0x%X SUCCESSFUL\n", address);
        bContinue = 0;
        continue;
      }

      if(status != BUSY_IDLE){
        printf("Write to addr: 0x%X failed. status = 0x%X. \n", address, status);
        bContinue = 1;
        continue;
      }
    }


    // show percentage
    {
      static int last_percent = 0;
      int total = cfm_end_addr - cfm_start_addr;
      int percent = ((address+4 - cfm_start_addr)*100)/total;

      if(last_percent != percent) {
        last_percent = percent;
        printf("\rProgress: Addr: 0x%X  [%d %%]", address, percent);
        fflush(stdout);
      }
    }

    offset+= 4;
  }

  printf("\n Done!\n");
  max10_protect_sectors();

  return ret;
}

/*Dual configuration IP on MAX 10*/

int max10_dual_boot_busy(void)
{
  int value;

  // offset 3 register.
  value = max10_reg_read(g_dual_boot_base + 3*4);
  if(value == 0)
    return 0;
  else
    return 1;
}

uint8_t max10_dual_boot_cfg_sel_get(void)
{
  int value;

  // offset 7 register.
  value = max10_reg_read(g_dual_boot_base + 7*4);

  if(value & (0x1<<1))
    return CONFIG_1;
  else
    return CONFIG_0;
}

int max10_dual_boot_cfg_sel_set(uint8_t img_no)
{
  int value;
  int address = g_dual_boot_base + 4;  // offset 1
  int ret;

  // offset 1 (write only), bit 0, config_sel_overwrite
  value = 0x1;  //config_sel_overwrite = 1

  switch(img_no) {
    case CONFIG_0:
      break;
    case CONFIG_1:
      value |= 0x1<<1;
      break;
  }

  if(max10_dual_boot_busy() == 1) {
    return -1;
  }

  ret = max10_reg_write(address, value);
  return ret;
}

int max10_trig_reconfig(void)
{
  int value = 0x1;
  int address = g_dual_boot_base + 0;  // offset 0
  int ret;

  if (max10_dual_boot_busy() == 0)
    ret = max10_reg_write(address, value);
  else
    return -1;

  return ret;
}

int max10_cpld_cfm_update(FILE *fd, char *key, char is_signed)
{
  struct stat finfo;
  char buf[SIGNED_FULL_SIZE] = {0};
  char str[SIGNED_FULL_SIZE] = {0};
  uint8_t *rpd_file_buff;
  uint8_t image_type;
  int rpd_filesize;
  int readbytes;
  int cfm_start_addr, cfm_end_addr;
  int ret = 0;  
  int flash_size;

  if (is_signed == true) {
    memcpy(str, key, 32);
    fseek(fd, -(SIGNED_DATA_SIZE), SEEK_END);
    if (fread(buf, 1, SIGNED_FULL_SIZE, fd)) {
      if (strstr(buf, str) == NULL) {
        printf("Error, Signed Key not Match\n");
        return -1;
      }
    } else {
       printf("File read fail\n");
       return -1;
    }
    fseek(fd, 0, SEEK_SET);
    printf("Singed Key Match\n");
  }

  // Get file size
  fstat(fileno(fd), &finfo);
  if (is_signed == true) {
    rpd_filesize = finfo.st_size - SIGNED_FULL_SIZE;
  } else {
    rpd_filesize = finfo.st_size;
  }
  printf("rpd file size = %d bytes. \r\n", rpd_filesize);

  // allocate memory
  rpd_file_buff = malloc(rpd_filesize);
  if (rpd_file_buff == 0) {
    printf("allocate memory fail. \r\n");
    return -1;
  }

  //read rpt file into memory
  readbytes = read(fileno(fd), rpd_file_buff, rpd_filesize);
  if (readbytes != rpd_filesize) {
    printf("read(), ret = %d. \r\n", readbytes);
  }

  cfm_start_addr = g_cfm_start_addr;
  cfm_end_addr = g_cfm_end_addr;
  image_type = g_cfm_image_type;

  flash_size = cfm_end_addr - cfm_start_addr +1;
  if(rpd_filesize != flash_size) {
    printf("Error, File Size=%x Flash Size=%x\n", rpd_filesize, flash_size); 
    return -1;
  } 

  ret = max10_update_rpd(rpd_file_buff, image_type, cfm_start_addr, cfm_end_addr);
  free(rpd_file_buff);

  return ret;
}

int max10_cpld_get_id(uint32_t *dev_id)
{
  *dev_id = 0x01234567;
  return 0;
}

static void max10_iic_init(uint8_t bus, uint8_t addr)
{
  snprintf(g_i2c_file_dp, sizeof(g_i2c_file_dp), "/dev/i2c-%u", bus);
  g_i2c_bridge_addr = addr;
  return;
}

static void max10_dev_init(altera_max10_attr_t *attr)
{
  g_flash_csr_base = attr->csr_base;
  g_flash_csr_status_reg = g_flash_csr_base + 0x00;
  g_flash_csr_ctrl_reg = g_flash_csr_base + 0x04;
  g_flash_data_reg = attr->data_base;
  g_dual_boot_base = attr->boot_base;
  g_cfm_start_addr = attr->start_addr;
  g_cfm_end_addr = attr->end_addr;
  g_cfm_image_type = attr->img_type;

  DEBUGMSG("%s file dp=%s\n", __func__, g_i2c_file_dp);
  DEBUGMSG("base reg=%x, status_reg=%x, ctrl_reg=%x\n", g_flash_csr_base, g_flash_csr_status_reg, g_flash_csr_ctrl_reg);
  DEBUGMSG("cfm start addr=%x, endaddr=%x\n", g_cfm_start_addr, g_cfm_end_addr);
  return;
}

static int cpld_dev_open(void *_attr)
{
  int i2c_file;
  altera_max10_attr_t *attr = (altera_max10_attr_t *)_attr;
  if (!attr) {
    return -1;
  }

  max10_dev_init(attr);
  max10_iic_init(attr->bus_id, attr->slv_addr);

  // Open a connection to the I2C userspace control file.
  if ((i2c_file = open(g_i2c_file_dp, O_RDWR)) < 0) {
    printf("Unable to open %s\n", g_i2c_file_dp);
    return -1;
  }

  max10_update_init(i2c_file);

  return 0;
}

static int cpld_dev_close()
{
  max10_done();
  return 0;
}

static int max10_get_fw_ver(uint32_t *ver)
{
  int ret;

  ret = get_i2c_register(g_i2c_file, g_i2c_bridge_addr, 0x00100028, ver);
  if (ret != 4) {
    syslog(LOG_WARNING, "get_i2c_register() ERROR. ret = %d.", ret);
    return -1;
  }

  return 0;
}

struct cpld_dev_info altera_dev_list[] = {
  [0] = {
    .name = "MAX10-10M16",
    .dev_id = 0x01234567,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = max10_get_fw_ver,
    .cpld_program = max10_cpld_cfm_update,
    .cpld_dev_id = max10_cpld_get_id,
  },
  [1] = {
    .name = "MAX10-10M25",
    .dev_id = 0x01234567,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = max10_get_fw_ver,
    .cpld_program = max10_cpld_cfm_update,
    .cpld_dev_id = max10_cpld_get_id,
  },
  [2] = {
    .name = "MAX10-10M04",
    .dev_id = 0x01234567,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = max10_get_fw_ver,
    .cpld_program = max10_cpld_cfm_update,
    .cpld_dev_id = max10_cpld_get_id,
  },
  [2] = {
    .name = "MAX10-10M08",
    .dev_id = 0x01234567,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = max10_get_fw_ver,
    .cpld_program = max10_cpld_cfm_update,
    .cpld_dev_id = max10_cpld_get_id,
  },
};
