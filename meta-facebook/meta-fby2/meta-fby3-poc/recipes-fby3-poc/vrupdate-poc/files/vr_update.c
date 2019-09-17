#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <facebook/bic.h>

#define TI_GET_DEVICE_ID    0xAD
#define TI_USER_NVM_INDEX   0xF5
#define TI_USER_NVM_EXECUTE 0xF6
#define TI_NVM_CHECKSUM     0xF0
#define TI_NVM_INDEX_00     0x00

#define MAX_RETRY  3
#define DATA_SIZE 66 /*(66 chars / 2) = 33 bytes*/
#define TI_TOTAL_NVM_INDEX 0x9
#define TI_PER_PKT_SIZE    0x21

#define USAGE_MESSAGE \
        "Usage:\n" \
        "  %s --sb slot[1|2|3|4] $filename.csv \n\n"

unsigned char show_verbose = 0;

struct dev_table {
  unsigned char addr;
  unsigned char *dev_name;
} dev_list[] = {
  {0x60, "VCCIN/VSA"},
  {0x62, "VCCIO"},
  {0x64, "VDDQ_ABC"},
  {0x66, "VDDQ_DEF"},
  {NULL, NULL},
};

static int dev_table_size = (sizeof(dev_list)/sizeof(struct dev_table));

struct devinfo {
  unsigned char addr;
  unsigned char crc16[2];
  unsigned char devid[6];
  unsigned char data[297];
};

//CRC16 lookup table 0x8005
unsigned int CRC16_LUT[256] = {
  0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011, 0x8033, 0x0036, 0x003c, 0x8039,
  0x0028, 0x802d, 0x8027, 0x0022, 0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
  0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041, 0x80c3, 0x00c6, 0x00cc, 0x80c9,
  0x00d8, 0x80dd, 0x80d7, 0x00d2, 0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
  0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1, 0x8093, 0x0096, 0x009c, 0x8099,
  0x0088, 0x808d, 0x8087, 0x0082, 0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
  0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1, 0x01e0, 0x81e5, 0x81ef, 0x01ea,
  0x81fb, 0x01fe, 0x01f4, 0x81f1, 0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
  0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151, 0x8173, 0x0176, 0x017c, 0x8179,
  0x0168, 0x816d, 0x8167, 0x0162, 0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
  0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101, 0x8303, 0x0306, 0x030c, 0x8309,
  0x0318, 0x831d, 0x8317, 0x0312, 0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
  0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371, 0x8353, 0x0356, 0x035c, 0x8359,
  0x0348, 0x834d, 0x8347, 0x0342, 0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
  0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2, 0x83a3, 0x03a6, 0x03ac, 0x83a9,
  0x03b8, 0x83bd, 0x83b7, 0x03b2, 0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
  0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291, 0x82b3, 0x02b6, 0x02bc, 0x82b9,
  0x02a8, 0x82ad, 0x82a7, 0x02a2, 0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
  0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1, 0x8243, 0x0246, 0x024c, 0x8249,
  0x0258, 0x825d, 0x8257, 0x0252, 0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
  0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231, 0x8213, 0x0216, 0x021c, 0x8219,
  0x0208, 0x820d, 0x8207, 0x0202
};

static int cal_crc16(struct devinfo *vr_dev) {
  unsigned char data[254] = {0};
  unsigned char data_index = 0;
  unsigned int crc16_accum;
  unsigned int crc_shift;
  unsigned char index;
  int i;

  //get the data and checksum
  for (i = 0; i < TI_TOTAL_NVM_INDEX; i++ ) {
    if ( i == 0 ) {
      memcpy(vr_dev->crc16, &vr_dev->data[10], 2); //index10~11 is the checksum
      memcpy(&data[data_index], &vr_dev->data[12], 21); //get data
      data_index += 21;
    } else if ( i == 8 ) { //get the last data
      memcpy(&data[data_index], &vr_dev->data[(i*TI_PER_PKT_SIZE)+1], 9);
      data_index += 9;
    } else {
      memcpy(&data[data_index], &vr_dev->data[(i*TI_PER_PKT_SIZE)+1], 32);
      data_index += 32;
    }
  }

  crc16_accum = 0;
  crc_shift = 0;
  for(i=0; i<254; i++)
  {
    index = ((crc16_accum >> 8) ^ data[i]) & 0xFF;
    crc_shift = (crc16_accum << 8) & 0xFFFF;
    crc16_accum = (crc_shift ^ CRC16_LUT[index]) & 0xFFFF;
  }
  return ( crc16_accum == (vr_dev->crc16[1] << 8 | vr_dev->crc16[0]))?1:-1;
}

static int search_dev(unsigned char addr) {
  int i;
  for ( i = 0; i< dev_table_size; i++ ) {
    if ( addr == dev_list[i].addr ) {
      printf("Update: %s, ", dev_list[i].dev_name);
      return 0;
    }
  }

  return -1;
}

int send_vr_pmbus_data(unsigned char slot_id, unsigned char addr, unsigned char cmd, unsigned char readcnt, unsigned char *data, int data_len, unsigned char *rxbuf, unsigned char *rxlen) {
  int ret;
  int retries = MAX_RETRY;
  unsigned char txbuf[64] = {0x11/*bus*/, addr << 1/*addr*/, readcnt/*read cnt*/, cmd};
  unsigned char txlen = 4; //start from 4

  if ( data_len > 0 ) {
    memcpy(&txbuf[txlen], data, data_len);
  }

  txlen += data_len;

  if ( show_verbose == 1 ) {
    int i;
    printf("Send: ");
    for ( i = 0; i < txlen; i++ ) {
      printf("%02X ", txbuf[i]);
    }
    printf("\n");
  }

  while ( retries > 0 ) {
    ret = bic_ipmb_wrapper(slot_id, NETFN_APP_REQ, CMD_APP_MASTER_WRITE_READ, txbuf, txlen, rxbuf, rxlen);
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

static int update_vr(unsigned char slot_id, struct devinfo *vr_dev) {
  int ret;
  int i,j, check_cnt = 0;
  unsigned char tbuf[32] = {0}; //data to be sent
  unsigned char rbuf[32] = {0};
  unsigned char rlen = 0;
  unsigned char tlen = 0;
  unsigned char addr = vr_dev->addr;
  unsigned char user_nvm_index_0 = TI_NVM_INDEX_00;
  unsigned char user_num_execute = TI_USER_NVM_EXECUTE;

  //step 0 - check dev id first
  ret = send_vr_pmbus_data(slot_id, addr, TI_GET_DEVICE_ID, 0x07, NULL, 0, rbuf, &rlen);
  if ( ret < 0 || rlen == 0 ) {
    printf("Cannot get the device id, ret=%d, length=%d\n", ret, rlen);
    return ret;
  }
 
  ret = memcmp(&rbuf[1], vr_dev->devid, sizeof(vr_dev->devid));
  if ( ret != 0 ) {
    printf("Device ID is not match\n");
    return -1;    
  } 
 
  //step 1 - set index to 0x0
  ret = send_vr_pmbus_data(slot_id, addr, TI_USER_NVM_INDEX, 0x00, (unsigned char *)&user_nvm_index_0/*index*/, 1, rbuf, &rlen);
  if ( ret < 0 ) {
    printf("Cannot initialize the page to 0x00!\n");
    return ret;
  }

  //step 2 - write data from index 0 to 9
  for (i = 0; i < TI_TOTAL_NVM_INDEX; i++ ) {
    static int progress = TI_PER_PKT_SIZE;
    ret = send_vr_pmbus_data(slot_id, addr, TI_USER_NVM_EXECUTE, 0x00, &vr_dev->data[i*TI_PER_PKT_SIZE], TI_PER_PKT_SIZE, rbuf, &rlen);
    if ( ret < 0 ) {
      printf("Cannot write data to index %d\n", i);
      printf("Data: ");
      for ( j = i*TI_PER_PKT_SIZE; j < (i*TI_PER_PKT_SIZE)+TI_PER_PKT_SIZE; j++ ) {
        printf("%02X", vr_dev->data[j]);
      }
      printf("\n");
      return ret;
    }
    progress += TI_PER_PKT_SIZE;
    printf("Update: %d %%\n", progress*10/TI_PER_PKT_SIZE);
  }

  //step 3 - set index to 0x0
  ret = send_vr_pmbus_data(slot_id, addr, TI_USER_NVM_INDEX, 0x00, (unsigned char *)&user_nvm_index_0/*index*/, 1, rbuf, &rlen);
  if ( ret < 0 ) {
    printf("Cannot set page to 0x00!\n");
    return ret;
  }

  //step 4 - verify the data from 0 to 9
  for (i = 0; i < TI_TOTAL_NVM_INDEX; i++ ) {
    ret = send_vr_pmbus_data(slot_id, addr, TI_USER_NVM_EXECUTE, TI_PER_PKT_SIZE, NULL, 0, rbuf, &rlen);
    if ( rlen > 0 ) {
      if ( show_verbose == 1 ) {
        printf("[%d]Rsp: ", rlen);
        for ( j = 0; j < TI_PER_PKT_SIZE; j++ ) {
          printf("%02X ", rbuf[j]);
        }
        printf("\n[%d]Cmp: ", rlen, TI_PER_PKT_SIZE);
        for ( j = 0; j < TI_PER_PKT_SIZE; j++ ) {
          printf("%02X ", vr_dev->data[(i*TI_PER_PKT_SIZE)+j]);
        }
        printf("\n");
      }

      ret = memcmp(rbuf, &vr_dev->data[i*TI_PER_PKT_SIZE], TI_PER_PKT_SIZE);
      if ( ret != 0 ) {
        printf("Data of index %d is not expected!\n", i);
      } else {
        check_cnt++;
      } 
    }
  }

  if ( check_cnt != TI_TOTAL_NVM_INDEX ) {
    printf("Update VR fails\n. There are only %d expected data\n", check_cnt);
    return -1;
  }

  //step 5 - wait 100ms and do power cycle?
}

static void flash_data_parser(char *path, struct devinfo *vr_dev) {
#define IC_DEVICE_ID "IC_DEVICE_ID"
#define BLOCK_READ "BlockRead"
#define BLOCK_WRITE "BlockWrite"

  FILE *fp;
  char *line = NULL;
  char *token = NULL;
  size_t len = 0;
  ssize_t read;
  struct devinfo dev;
  char temp[128] = {0};
  char data[3] = {0};
  long int temp_val;
  int index = 0;
  int data_idx;

  if ( (fp = fopen(path, "r")) == NULL ) {
    printf("Cannot open file!\n");
    exit(EXIT_FAILURE);
  }

  while ( (read = getline(&line, &len, fp)) != -1 ) {
    memset(temp,'\0', sizeof(temp));
    if ( (token = strstr(line, IC_DEVICE_ID)) != NULL ) { //get device id from the string
      token = strstr(token, "0x"); //get the pointer
      strncpy(temp, token+2, 12); //copy 0x'XXXXXXXXXXXX' to temp
      int devid_idx = 0;
      for ( data_idx = 0; data_idx < strlen(temp); data_idx += 2 ) {
        memset(data, '\0', sizeof(data));
        strncpy(data, &temp[data_idx], 2);
        vr_dev->devid[devid_idx++] = (unsigned char)strtol(data, NULL, 16);
      }
    } else if ( (token = strstr(line, BLOCK_READ)) != NULL ) { //get block read
      token = strstr(token, ",");
      strncpy(temp, token + 1, 4); //copy 0xXX to temp
      temp_val = strtol(temp, NULL, 16);
      vr_dev->addr = (unsigned char)temp_val;
    } else if ( (token = strstr(line, BLOCK_WRITE)) != NULL ) { //get block write
      token = strstr(token, ","); 
      strncpy(temp, &token[13], DATA_SIZE); //copy 32 bytes to temp
      for ( data_idx = 0; data_idx < DATA_SIZE; data_idx += 2) {
        memset(data, '\0', sizeof(data));
        strncpy(data, &temp[data_idx], 2);
        vr_dev->data[index++] = (unsigned char)strtol(data, NULL, 16);
      }
    }
  }
}

static void usage(char *str)
{
  printf(USAGE_MESSAGE, str);
}

int main(int argc, char **argv) {
  int ret;
  int slot_id = 0;
  struct timeval start, end;
  FILE *fp;
  char fpath[128];
  struct devinfo vr_dev;

  printf("================================================\n");
  printf("Update VR via I2C interface,  v1.0\n");
  printf("Build date: %s %s \r\n", __DATE__, __TIME__);
  printf("================================================\n");

  gettimeofday(&start, NULL);

  if(argc < 3) {
    usage(argv[0]);
    return -1;
  }

  // extract parameter
  if ( strcmp(argv[1], "--sb") == 0 ) {
    if ( (strcmp(argv[2], "slot1") == 0) ) {
      slot_id = 1;
    } else if ( (strcmp(argv[2], "slot2") == 0) ) {
      slot_id = 2;
    } else if ( (strcmp(argv[2], "slot3") == 0) ) {
      slot_id = 3;
    } else if ( (strcmp(argv[2], "slot4") == 0) ) {
      slot_id = 4;
    } else {
      printf("Cannot recognize the unknown slot: %s\n", argv[2]);
      return -1;
    }
  } else {
    printf("Cannot recognize the param - %s\n", argv[1]);
    return 0;
  }

  if ( argc == 5 && (strcmp(argv[4], "-v") == 0) ) {
    show_verbose = 1;
  }

  strcpy(fpath, argv[3]);
  printf("File: %s, ", fpath);
  memset(&vr_dev, 0, sizeof(vr_dev));

  flash_data_parser(fpath, &vr_dev);
  
  if ( show_verbose == 1 ) {
    int i;
    printf("slave addr: 0x%x, ", vr_dev.addr);
    printf("device ID: 0x");
    for ( i = 0; i < sizeof(vr_dev.devid); i++ ) {
      printf("%02X", vr_dev.devid[i]);
    }
    printf("\n");

    for ( i = 0 ; i < sizeof(vr_dev.data); i++ ) {
      printf(" %02X ", vr_dev.data[i]);
      if ( (i+1) % 33 == 0 ) printf("\n");
    }
  }

  ret = search_dev(vr_dev.addr);
  if ( ret < 0 ) {
    printf("The dev is not in the list!\n");
    return -1;
  }

  ret = cal_crc16(&vr_dev);
  if ( ret < 0 ) {
    printf("The checksum of flash data is incorrect.");
    return -1;
  }

  printf("Checksum: 0x%04X\n", (vr_dev.crc16[1] << 8| vr_dev.crc16[0]));

  update_vr(slot_id, &vr_dev);

  gettimeofday(&end, NULL);

  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  return 0;
}
