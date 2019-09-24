#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <facebook/bic.h>

#define MAX_RETRY  500
#define LATTICE_COL_SIZE 128

int show_verbose = 0;

#define SHOW_MSG(x, msg_, ...)                   \
               if ( x == 1 ) {                   \
                   printf((msg_), ##__VA_ARGS__);\
               }                                 \

#define SHOW_BYTE(x, end, data)             \
               if ( x == 1 ) {              \
                 int i;                     \
                 for (i=0;i<end;i++) {      \
                   printf("%02X ", data[i]);\
                 }                          \
                 printf("\n");              \
               }                            \

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

static void
msleep(int msec) {
  struct timespec req;

  req.tv_sec = 0;
  req.tv_nsec = msec * 1000 * 1000;

  while(nanosleep(&req, &req) == -1 && errno == EINTR) {
    continue;
  }
}

static int
send_cpld_data(unsigned char slot_id, unsigned char intf, unsigned char addr, unsigned char *data, unsigned char data_len, unsigned char *rsp, unsigned char read_cnt) {
  int ret;
  int retries = 3;
  unsigned char txbuf[256] = {0x9c, 0x9c, 0x00, intf, NETFN_APP_REQ << 2, CMD_APP_MASTER_WRITE_READ, 0x01/*bus 1*/, addr, read_cnt/*read cnt*/};
  unsigned char txlen = 9;//start from 9
  unsigned char rxbuf[256] = {0};
  unsigned char rxlen = 0;

  if ( data_len > 0 ) {
    memcpy(&txbuf[txlen], data, data_len);
    txlen += data_len;
  }

  SHOW_MSG(show_verbose, "Send: ");
  SHOW_BYTE(show_verbose, txlen, txbuf);

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

  return ret;
    
}

static int
read_cpld_status_flag(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0x3C, 0x0, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 4;
  
  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);

  if ( ret < 0 ) {
    printf("Couldn't read_cpld_status_flag cmd\n");
    return ret;
  }

  SHOW_MSG(show_verbose, "status: ");
  SHOW_BYTE(show_verbose, read_cnt, rsp);

  rsp[0] = (rsp[2] >> 12) & 0x3;

  ret = (rsp[0] == 0x0)?0:-1;

  return ret;
}

static int
read_cpld_busy_flag(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0xF0, 0x0, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 1;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send read_cpld_busy_flag cmd\n");
    return ret;
  }

  SHOW_MSG(show_verbose, "busy: ");
  SHOW_BYTE(show_verbose, read_cnt, rsp);

  rsp[0] = (rsp[0] & 0x80) >> 7;

  ret = (rsp[0] == 0x0)?0:-1;

  return ret;
}

static int
set_mux_to_cpld(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[1] = {0x2};
  unsigned char data_len = 1;
  ret = send_cpld_data(slot_id, intf, addr, data, data_len, NULL, 0);
  return ret;
}

static int
read_cpld_dev_id(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0xE0, 0x0, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 4;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    return ret;
  }

  int i;
  printf("DevID: ");
  for (i=0;i<read_cnt;i++)
    printf("%02X", rsp[i]);
  printf("\n");
  
  return ret;
}

static int
enter_transparent_mode(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  int retry = MAX_RETRY; 
  unsigned char data[3]= {0x74, 0x08, 0x00};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;

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
erase_flash(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  int retry = MAX_RETRY;
  unsigned char data[4]= {0x0E, 0x04/*erase CF*/, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;

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
reset_config_addr(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0x46, 0x00, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send reset_config_addr cmd\n");
    return ret;
  }

  return ret;
}

static int
program_cf(unsigned char slot_id, unsigned char intf, unsigned char addr, unsigned char *cf_data) {
  int ret;
  int retry = MAX_RETRY;
  unsigned char data[20]= {0x70, 0x0, 0x0, 0x01};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;
  int idx;

  for (idx = 4; idx <= 16; idx += 4) {
    data[idx + 0] = cf_data[idx - 1];
    data[idx + 1] = cf_data[idx - 2];
    data[idx + 2] = cf_data[idx - 3];
    data[idx + 3] = cf_data[idx - 4];
  }

  SHOW_BYTE(show_verbose, data_len, data);

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

static int
verify_cf(unsigned char slot_id, unsigned char intf, unsigned char addr, unsigned char *cf_data) {
  int ret;
  int retry = MAX_RETRY;
  unsigned char data[4]= {0x73, 0x00, 0x00, 0x01};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char cmp_data[16] = {0};
  unsigned char read_cnt = sizeof(rsp);
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

  SHOW_MSG(show_verbose, "recv: ");
  SHOW_BYTE(show_verbose, read_cnt, rsp);

  SHOW_MSG(show_verbose, "comp: ");
  SHOW_BYTE(show_verbose, read_cnt, cmp_data);
 
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
program_user_code(unsigned char slot_id, unsigned char intf, unsigned char addr, unsigned char *user_data) {
  int ret;
  int retry = MAX_RETRY;
  unsigned char data[8]= {0xC2, 0x0, 0x0, 0x00};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;

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
program_done(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0x5E, 0x0, 0x0, 0x00};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 0;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  if ( ret < 0 ) {
    printf("Couldn't send program_done cmd\n");
    return ret;
  }

  return ret;
}

static uint8_t
read_cpld_user_code(unsigned char slot_id, unsigned char intf, unsigned char addr) {
  int ret;
  unsigned char data[4]= {0xC0, 0x0, 0x0, 0x0};
  unsigned char data_len = sizeof(data);
  unsigned char rsp[16] = {0};
  unsigned char read_cnt = 4;

  ret = send_cpld_data(slot_id, intf, addr, data, data_len, rsp, read_cnt);
  int i;
  printf("Usercode: ");
  for (i=0;i<read_cnt;i++)
    printf("%02X", rsp[i]);
  printf("\n");

  return ret;
}

static int 
detect_cpld_dev(unsigned char slot_id, unsigned char intf) { 
  int retries = 100;
  int ret;
  printf("Detect CPLD...");
  fflush(stdout);
  do {

    ret = set_mux_to_cpld(slot_id, intf, MUX);

    ret = read_cpld_dev_id(slot_id, intf, CPLD);

    if ( ret < 0 ) {
      msleep(10);
    } else {
      break;   
    }
  } while(retries--);

  if ( ret < 0 ) {
    printf("No Device\n");
  }

  return ret;
}

/*reverse byte order*/
static unsigned char 
reverse_bit(unsigned char b) {
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

    result[result_index] |= ((unsigned char)data[i] << data_index);

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
  const char TAG_UFM[]="NOTE TAG DATA";
  int ret = 0;
  int i = 0;
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
      printf("[QF]%ld\n",dev_info->QF);
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
          printf("[%d]%x %x %x %x\n",dev_info->CF_Line, dev_info->CF[current_addr],dev_info->CF[current_addr+1],dev_info->CF[current_addr+2],dev_info->CF[current_addr+3]);
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
          printf("[%s]CF Line: %d\n", __func__, dev_info->CF_Line);
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
          printf("[%s]UFM Line: %d\n", __func__, dev_info->UFM_Line);
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

int main(int argc, char **argv) {
  enum {
    UPDATE_FW = 0x1,
    SHOW_VER  = 0x2,
  };

  int ret;
  uint32_t fsize = 0;
  uint32_t record_offset = 0;
  CPLDInfo dev_info;
  struct timeval start, end;
  FILE *fp;
  char fpath[128];
  unsigned char slot_id = 0x1;
  const unsigned char intf = 0x5;
  unsigned char addr = 0xe2;
  unsigned char action = 0x0;

  printf("================================================\n");
  printf("Update CPLD firmware,  v1.0\n");
  printf("Build date: %s %s \r\n", __DATE__, __TIME__);
  printf("================================================\n");

  gettimeofday(&start, NULL);

  if ( argc < 3 ) {
    printf("Please check the params\n");
    printf("%s $slot --show\n", argv[0]);
    printf("%s $slot $file.jed\n", argv[0]);
    return 0;
  }

  if ( (strcmp(argv[1], "slot1") == 0) ) {
    slot_id = 1;
  } else if ( (strcmp(argv[1], "slot2") == 0) ) {
    slot_id = 2;
  } else if ( (strcmp(argv[1], "slot3") == 0) ) {
    slot_id = 3;
  } else if ( (strcmp(argv[1], "slot4") == 0) ) {
    slot_id = 4;
  } else {
    printf("Cannot recognize the unknown slot: %s\n", argv[1]);
    return -1;
  }
 
  if ( (strcmp(argv[2], "--show") == 0) ) {
    action = SHOW_VER;
  } else {
    action = UPDATE_FW;
  }

  if ( (argc == 4) && (strcmp(argv[3], "-v") == 0) ) {
    show_verbose = 1;
  }

  if ( action == UPDATE_FW ) {
    strcpy(fpath, argv[2]);
    printf("Input: %s\n", fpath); 

    fp = fopen(fpath,"r");
    if ( fp == NULL ) {
      printf("Invalid file: %s\n", fpath);
      return -1;
    }

    memset(&dev_info, 0, sizeof(dev_info));
    ret = LCMXO2Family_JED_File_Parser(fp, &dev_info);
    if ( ret < 0 ) {
      return ret;
    }

    fsize = dev_info.CF_Line / 20;
  }

  //step 1 - detect CPLD
  ret = detect_cpld_dev(slot_id, intf);
  if ( ret < 0 ) {
    goto exit;
  }

  //step 2 - read the user code
  addr = CPLD;
  if ( action == SHOW_VER ) {
    SHOW_MSG(show_verbose, "Read the user code\n");
    ret = read_cpld_user_code(slot_id, intf, addr);
    return ret;
  }
   
  //step 3 - enter transparent mode and check the status
  SHOW_MSG(show_verbose, "Enter transparent mode\n");
  ret = enter_transparent_mode(slot_id, intf, addr);

  //step 4 - erase the CF and check the status
  SHOW_MSG(show_verbose, "Erase flash\n");
  ret = erase_flash(slot_id, intf, addr);

  //step 6 - Transmit Reset Configuration Address
  SHOW_MSG(show_verbose, "Reset config\n");
  ret = reset_config_addr(slot_id, intf, addr);

  //step 7 - send data and check the status everytime
  int i,j,data_idx;
  for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
    ret = program_cf(slot_id, intf, addr, (unsigned char *)&dev_info.CF[data_idx]); 
    if ( ret < 0 ) {
      printf("send cf data but ret < 0. exit.");
      goto exit;
    }
    
    if ( (record_offset + fsize) <= i ) {
        printf("updated cpld: %d %%\n", (i/fsize)*5);
        record_offset += fsize;
    }
  }

  //step 8 - program user code
  SHOW_MSG(show_verbose, "Program user code\n");
  ret = program_user_code(slot_id, intf, addr, (unsigned char *)&dev_info.Version);

  //step 9 - read the user code
  SHOW_MSG(show_verbose, "Read the user code\n");
  if ( show_verbose )
    ret = read_cpld_user_code(slot_id, intf, addr);

  //step 10 - verify the CF data 
  //step 10.1 Transmit Reset Configuration Address again
  SHOW_MSG(show_verbose, "Reset config\n");
  ret = reset_config_addr(slot_id, intf, addr);

  record_offset = 0;
  //step 10.2 Transmit Read Command with Number of Pages
  for (i = 0, data_idx = 0; i < dev_info.CF_Line; i++, data_idx+=4) {
    ret = verify_cf(slot_id, intf, addr, (unsigned char *)&dev_info.CF[data_idx]);
    if ( ret < 0 ) {
      printf("Read cf data but ret < 0. exit\n");
      goto exit;
    }

    if ( (record_offset + fsize) <= i ) {
      printf("verify cpld: %d %%\n", (i/fsize)*5);
      record_offset += fsize;
    }
  }

  //step 11 - program done
  SHOW_MSG(show_verbose, "Program done\n");
  ret = program_done(slot_id, intf, addr);

  if ( dev_info.CF != NULL ) free(dev_info.CF);

exit: 
  gettimeofday(&end, NULL);
  printf("Elapsed time:  %d   sec.\n", (int)(end.tv_sec - start.tv_sec));

  return ret;
}
