#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <facebook/bic.h>

#define USAGE_MESSAGE \
    "Usage:\n" \
    "  %s --bic --[feb|reb|bb] slot[1|2|3|4] $image  \n" \
    " feb: front expansion board \n" \
    " reb: riser expansion board \n" \
    "  bb: baseboard\n"

//define the command and its size here
#define BIC_CMD_DOWNLOAD 0x21
#define BIC_CMD_DOWNLOAD_SIZE 11

#define BIC_CMD_RUN 0x22
#define BIC_CMD_RUN_SIZE 7

#define BIC_CMD_DATA 0x24
#define BIC_CMD_DATA_SIZE 0xFF

#define BIC_FLASH_START 0x8000

unsigned char show_verbose = 0;

enum {
  FEXP_BIC_I2C_WRITE   = 0x20,
  FEXP_BIC_I2C_READ    = 0x21,
  FEXP_BIC_I2C_UPDATE  = 0x22,
  FEXP_BIC_IPMI_I2C_SW = 0x23,

  REXP_BIC_I2C_WRITE   = 0x24,
  REXP_BIC_I2C_READ    = 0x25,
  REXP_BIC_I2C_UPDATE  = 0x26,
  REXP_BIC_IPMI_I2C_SW = 0x27,

  BB_BIC_I2C_WRITE     = 0x28,
  BB_BIC_I2C_READ      = 0x29,
  BB_BIC_I2C_UPDATE    = 0x2A,
  BB_BIC_IPMI_I2C_SW   = 0x2B
};

enum {
  I2C_100K = 0x0,
  I2C_1M,
};

static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }  
}

static void print_data(uint8_t netfn, uint8_t cmd, uint8_t *buf, uint8_t len) {
  int i;
  printf("send: 0x%x 0x%x ", netfn, cmd);
  for ( i = 0; i < len; i++) {
    printf("0x%x ", buf[i]);
  }

  printf("\n");
}

static void usage(char *str)
{
  printf(USAGE_MESSAGE, str);
}

static uint8_t get_checksum(uint8_t *buf, uint8_t len) {
  int i;
  uint8_t result = 0;
  for ( i = 0; i < len; i++ ) {
    result += buf[i];
  }

  return result;
}

static int
enable_remote_bic_update(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[10] = {0x9c, 0x9c, 0x00, intf, NETFN_OEM_1S_REQ << 2, CMD_OEM_1S_ENABLE_BIC_UPDATE, 0x9c, 0x9c, 0x00, 0x01/*flag - i2c*/};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;

  if ( show_verbose == 1 )
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf));

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf), rbuf, &rlen);
  return ret;
}

static int
setup_remote_bic_i2c_speed(uint8_t slot_id, uint8_t speed, uint8_t intf) {
  uint8_t tbuf[6] = {0x9c, 0x9c, 0x00, intf, 0x00, 0x00};
  uint8_t rbuf[16] = {0x00};
  uint8_t rlen = 0;
  int ret;

  tbuf[4] = speed;

  if ( show_verbose == 1 )
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf));

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, sizeof(tbuf), rbuf, &rlen);
  return ret;
}

static int
send_start_remote_bic_update(uint8_t slot_id, int size, uint8_t intf) {
  uint8_t tbuf[32] = {0x9c, 0x9c, 0x00, intf, BIC_CMD_DOWNLOAD_SIZE};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;

  //checksum
  tbuf[5] = 0x0;

  //command
  tbuf[6] = BIC_CMD_DOWNLOAD;

  //Provide the addr
  tbuf[7] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[8] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[9] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[10] = (BIC_FLASH_START) & 0xff;

  //Provide the image size
  tbuf[11] = (size >> 24) & 0xff;
  tbuf[12] = (size >> 16) & 0xff;
  tbuf[13] = (size >> 8) & 0xff;
  tbuf[14] = (size) & 0xff;

  //calculate the checksum
  tbuf[5] = get_checksum(&tbuf[6], BIC_CMD_DOWNLOAD_SIZE);
  tlen = 15;

  if ( show_verbose == 1 )
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);

  return ret;
}

static int
read_remote_bic_update_status(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[32] = {0x9c, 0x9c, 0x00, intf, 0x0, 0x0};
  uint8_t rbuf[32] = {0x00};
  uint8_t tlen = 6;
  uint8_t rlen = 0;
  int i;
  int ret;

  if ( show_verbose == 1 )
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  if ( rlen <= 0 ) {
      printf("[%s]status - invalid lenth %d!!", __func__, rlen);
      return -1;
  }

  return ret;
}

static int
send_complete_signal(uint8_t slot_id, uint8_t intf) {
  uint8_t tbuf[32] = {0x9c, 0x9c, 0x00, intf, BIC_CMD_RUN_SIZE};
  uint8_t rbuf[32] = {0};
  uint8_t tlen = 10;
  uint8_t rlen = 0;
  int ret;

  //checksum
  tbuf[5] = 0x0;

  tbuf[6] = BIC_CMD_RUN;
  tbuf[7] = (BIC_FLASH_START >> 24) & 0xff;
  tbuf[8] = (BIC_FLASH_START >> 16) & 0xff;
  tbuf[9] = (BIC_FLASH_START >> 8) & 0xff;
  tbuf[10] = (BIC_FLASH_START) & 0xff;

  //calculate the checksum
  tbuf[5] = get_checksum(&tbuf[6], BIC_CMD_DOWNLOAD_SIZE);
  tlen = 11;

  if ( show_verbose == 1 )
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  return ret;
}

static int
send_bic_image_data(uint8_t slot_id, uint16_t len, uint8_t *buf, uint8_t intf) {
  uint8_t tbuf[256] = {0x9c, 0x9c, 0x00, intf}; // IANA ID
  uint8_t rbuf[16] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int ret;
  int i;

  tbuf[4] = len + 3;
  tbuf[5] = get_checksum(buf, len) + BIC_CMD_DATA;
  tbuf[6] = BIC_CMD_DATA;

  memcpy(&tbuf[7], buf, len);
  tlen = len + 7;  

  if ( show_verbose == 1 ) {
    static int cnt = 0;
    printf("[%d]", cnt++);
    print_data(NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen);
  }

  ret = bic_ipmb_wrapper(slot_id, NETFN_OEM_1S_REQ, CMD_OEM_1S_MSG_OUT, tbuf, tlen, rbuf, &rlen);
  return (rbuf[4] == 0x0)?-1:0;
}

int main(int argc, char **argv) {
  int i2c_file;
  int fw_file;
  int file_size;
  struct stat finfo;
  int read_bytes, bytes_per_read = 0;
  uint8_t buf[256] = {0};
  struct timeval start, end;
  int ret;
  uint8_t target;
  uint8_t bmc_location = 0;
  uint8_t slot_id = 1;
  uint8_t write_if  = 0x0;
  uint8_t read_if   = 0x0;
  uint8_t update_if = 0x0; 
  uint8_t i2c_if    = 0x0;
  uint8_t intf = 0;
  uint16_t read_cnt;
  uint32_t dsize, last_offset, offset;

  printf("================================================\n");
  printf("Update the remote bic firmware via I2C interface,  v1.0\n");
  printf("Build date: %s %s \r\n", __DATE__, __TIME__);
  printf("================================================\n");

  if ( argc < 5 ) {
    printf("pls check the params\n"); 
    usage(argv[0]);
    return 0;
  }

  gettimeofday(&start, NULL);

  if ( !(strncmp(argv[1], "--bic", strlen(argv[1])) == 0) ) {
    usage(argv[0]);
    return -1;
  }

  if ( (strncmp(argv[2], "--feb", strlen(argv[2])) == 0) ) {
    intf = 0x5;
    write_if  = FEXP_BIC_I2C_WRITE;
    read_if   = FEXP_BIC_I2C_READ;
    update_if = FEXP_BIC_I2C_UPDATE;
    i2c_if    = FEXP_BIC_IPMI_I2C_SW;
  } else if ( (strncmp(argv[2], "--bb", strlen(argv[2])) == 0) ) {
    intf = 0x10;
    write_if  = BB_BIC_I2C_WRITE;
    read_if   = BB_BIC_I2C_READ;
    update_if = BB_BIC_I2C_UPDATE;
    i2c_if    = BB_BIC_IPMI_I2C_SW;
  } else if ( (strncmp(argv[2], "--reb", strlen(argv[2])) == 0) ) {
    intf = 0x15;
    write_if  = REXP_BIC_I2C_WRITE; 
    read_if   = REXP_BIC_I2C_READ;
    update_if = REXP_BIC_I2C_UPDATE;
    i2c_if    = REXP_BIC_IPMI_I2C_SW;
  }else {
    printf("Cannot recognize the target: %s\n", argv[2]);
    return -1;
  }

  if ( (strcmp(argv[3], "slot1") == 0) ) {
    slot_id = 1;
  } else if ( (strcmp(argv[3], "slot2") == 0) ) {
    slot_id = 2;
  } else if ( (strcmp(argv[3], "slot3") == 0) ) {
    slot_id = 3;
  } else if ( (strcmp(argv[3], "slot4") == 0) ) {
    slot_id = 4;
  } else {
    printf("Cannot recognize the unknown slot: %s\n", argv[3]);
    return -1;
  }

  // Open file
  printf("Open file: %s ", argv[4]);
  if ((fw_file = open(argv[4], O_RDONLY)) < 0) {
    printf(" Fail to open file: %s\n ", argv[4]);
    return -2;
  }

  if ( argc == 6 && (strcmp(argv[5], "-v") == 0) ) {
    show_verbose = 1;
  }

  
  // Get file size
  fstat(fw_file, &finfo);
  file_size = finfo.st_size;
  dsize = file_size / 20;

  bytes_per_read = 244;

  printf(",file size = %d bytes, read length = %d, slot = %d, intf = 0x%x\n", file_size, bytes_per_read, slot_id, intf);

  //step 1
  ret = enable_remote_bic_update(slot_id, intf);
  if ( ret < 0 ) {
    printf("Enable the remote bic update, Fail, ret = %d\n", ret);
    goto exit;
  }

  //step 2
  ret = setup_remote_bic_i2c_speed(slot_id, I2C_100K, i2c_if);
  if ( ret < 0 ) {
    printf("Set the speed of the remote bic to 100K, Fail, ret = %d\n", ret);
    goto exit;
  }

  //wait for it ready
  sleep(3);

  //step 3
  ret = send_start_remote_bic_update(slot_id, file_size, write_if);
  if ( ret < 0 ) {
    printf("Send start remote bic update, Fail, ret = %d\n", ret);
    goto exit;
  }

  //wait for it ready
  msleep(500);

  //step 4
  ret = read_remote_bic_update_status(slot_id, read_if);
  if ( ret < 0 ) {
    printf("Check remote bic update status, Fail, ret = %d\n", ret);
    goto exit;
  }
  
  //step 5
  offset = 0;
  last_offset = 0;
  read_bytes = 0;//re-init
  while (1) {
 
    read_bytes = read(fw_file, buf, bytes_per_read);
    if ( read_bytes <= 0 ) {
      break;
    }

    ret = send_bic_image_data(slot_id, read_bytes, buf, update_if);
    if ( ret < 0 ) {
      printf("Send data error -- exit!\n");
      goto exit;
    }

    offset += read_bytes;
    if ((last_offset + dsize) <= offset) {
      printf("updated bic: %d %%\n", (offset/dsize)*5);
      fflush(stdout);
      last_offset += dsize; 
    }
  }

  //step 6
  ret = send_complete_signal(slot_id, write_if);
  if ( ret < 0 ) {
    printf("Send complete signal, Fail, ret = %d\n", ret);
    goto exit;
  }

  //step 7
  ret = read_remote_bic_update_status(slot_id, read_if);
  if ( ret < 0 ) {
    printf("Get the bic status, Fail, ret = %d\n", ret);
    goto exit;
  }

exit:
  //step 8
  ret = setup_remote_bic_i2c_speed(slot_id, I2C_1M, i2c_if);
  if ( ret < 0 ) {
    printf("Set the speed of the remote bic to 1M, Fail, ret = %d\n", ret);
    goto exit;
  }

  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  // free memory
  close(fw_file);
  return ret;
}
