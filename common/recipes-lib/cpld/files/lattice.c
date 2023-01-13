#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "cpld.h"
#include "lattice.h"
#include "lattice_i2c.h"
#include "lattice_jtag.h"

//#define CPLD_DEBUG //enable debug message
//#define VERBOSE_DEBUG //enable detail debug message
struct cpld_dev_info lattice_dev_list[] = {
  {
    .name = "LCMXO2-2000HC",
    .dev_id = 0x012BB043,
    .intf = INTF_JTAG,
    .cpld_open = cpld_dev_open_jtag,
    .cpld_close = cpld_dev_close_jtag,
    .cpld_ver = common_cpld_Get_Ver_jtag,
    .cpld_program = LCMXO2Family_cpld_update_jtag,
    .cpld_dev_id = common_cpld_Get_id_jtag,
  },
  {
    .name = "LCMXO2-2000HC",
    .dev_id = 0x012BB043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO2-4000HC",
    .dev_id = 0x012BC043,
    .intf = INTF_JTAG,
    .cpld_open = cpld_dev_open_jtag,
    .cpld_close = cpld_dev_close_jtag,
    .cpld_ver = common_cpld_Get_Ver_jtag,
    .cpld_program = LCMXO2Family_cpld_update_jtag,
    .cpld_dev_id = common_cpld_Get_id_jtag,
  },
  {
    .name = "LCMXO2-4000HC",
    .dev_id = 0x012BC043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO2-7000HC",
    .dev_id = 0x012BD043,
    .intf = INTF_JTAG,
    .cpld_open = cpld_dev_open_jtag,
    .cpld_close = cpld_dev_close_jtag,
    .cpld_ver = common_cpld_Get_Ver_jtag,
    .cpld_program = LCMXO2Family_cpld_update_jtag,
    .cpld_dev_id = common_cpld_Get_id_jtag,
  },
  {
    .name = "LCMXO2-7000HC",
    .dev_id = 0x012BD043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO3-2100C",
    .dev_id = 0x612BB043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO3-4300C",
    .dev_id = 0x612BC043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO3_6900C",
    .dev_id = 0x612BD043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LCMXO3-9400C",
    .dev_id = 0x612BE043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = XO2XO3Family_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  },
  {
    .name = "LFMNX-50",
    .dev_id = 0x412E3043,
    .intf = INTF_I2C,
    .cpld_open = cpld_dev_open_i2c,
    .cpld_close = cpld_dev_close_i2c,
    .cpld_ver = common_cpld_Get_Ver_i2c,
    .cpld_program = NXFamily_cpld_update_i2c,
    .cpld_dev_id = common_cpld_Get_id_i2c,
  }
};


/*search the index of char in string*/
static int
indexof(const char *str, const char *c)
{
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
startWith(const char *str, const char *c)
{
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
ShiftData(char *data, unsigned int *result, int len)
{
  int i;
  int ret = 0;
  int result_index = 0, data_index = 0;
  int bit_count = 0;

#ifdef VERBOSE_DEBUG
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

#ifdef VERBOSE_DEBUG
    printf("[%s]%x %d %x\n", __func__, data[i], data_index, result[result_index]);
#endif
    data_index++;

    bit_count++;

    if ( 0 == ((i+1) % 32) )
    {
      data_index = 0;
#ifdef VERBOSE_DEBUG
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

/*check the size of cf and ufm*/
int
LCMXO2Family_Get_Update_Data_Size(FILE *jed_fd, int *cf_size, int *ufm_size)
{
  const char TAG_CF_START[] = "L000";
  int ReadLineSize = LATTICE_COL_SIZE;
  char tmp_buf[ReadLineSize];
  unsigned int CFStart = 0;
  unsigned int UFMStart = 0;
  const char TAG_UFM[] = "NOTE TAG DATA";
  int ret = 0;

  while( NULL != fgets(tmp_buf, ReadLineSize, jed_fd) )
  {
    if ( startWith(tmp_buf, TAG_CF_START/*"L000"*/) )
    {
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) )
    {
      UFMStart = 1;
    }

    if ( CFStart )
    {
      if ( !startWith(tmp_buf, TAG_CF_START/*"L000"*/) && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          (*cf_size)++;
        }
        else
        {
           CFStart = 0;
        }
      }
    }
    else if ( UFMStart )
    {
      if ( !startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) && !startWith(tmp_buf, "L") && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          (*ufm_size)++;
        }
        else
        {
          UFMStart = 0;
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

int
LCMXO2Family_JED_File_Parser(FILE *jed_fd, CPLDInfo *dev_info, int cf_size, int ufm_size)
{
  /**TAG Information**/
  const char TAG_QF[] = "QF";
  const char TAG_CF_START[] = "L000";
  const char TAG_UFM[] = "NOTE TAG DATA";
  const char TAG_ROW[] = "NOTE FEATURE";
  const char TAG_CHECKSUM[] = "C";
  const char TAG_USERCODE[] = "NOTE User Electronic";
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
  int cf_size_used = (cf_size * LATTICE_COL_SIZE) / 8; // unit: bytes
  int ufm_size_used = (ufm_size * LATTICE_COL_SIZE) / 8; // unit: bytes

  dev_info->CF = (unsigned int*)malloc( cf_size_used );
  memset(dev_info->CF, 0, cf_size_used);

  if ( ufm_size_used )
  {
    dev_info->UFM = (unsigned int*)malloc( ufm_size_used );
    memset(dev_info->UFM, 0, ufm_size_used);
  }

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
#ifdef VERBOSE_DEBUG
      printf("[%s][%zu][%c]", __func__, strlen(tmp_buf), tmp_buf[0]);
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
#ifdef VERBOSE_DEBUG
          printf("[%u]%x %x %x %x\n",dev_info->CF_Line, dev_info->CF[current_addr],dev_info->CF[current_addr+1],dev_info->CF[current_addr+2],dev_info->CF[current_addr+3]);
#endif
          //each data has 128bits(4*unsigned int), so the for-loop need to be run 4 times
          for ( i = 0; i < sizeof(unsigned int); i++ )
          {
            JED_CheckSum += (dev_info->CF[current_addr+i]>>24) & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i]>>16) & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i]>>8)  & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i])     & 0xff;
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
#ifdef VERBOSE_DEBUG
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

int
NX_Get_Update_Data_Size(FILE *jed_fd, int *cf_size, int *ufm_size)
{
  const char TAG_CF_START[] = "L000";
  const char TAG_UFM[] = "NOTE TAG DATA";
  const char TAG_EBR_START[] = "NOTE EBR_INIT DATA";
  const char TAG_FIRST_FUSE_ADDR[] = "L";
  const char TAG_END[] = "NOTE END CONFIG DATA";

  int ReadLineSize = LATTICE_COL_SIZE;
  char tmp_buf[ReadLineSize];
  unsigned int CFStart = 0;
  unsigned int UFMStart = 0;

  int ret = 0;

  while( NULL != fgets(tmp_buf, ReadLineSize, jed_fd) )
  {
    // printf("%s \n",tmp_buf);
    if ( startWith(tmp_buf, TAG_CF_START/*"L000"*/) )
    {
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) )
    {
      UFMStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_EBR_START) )
    {
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_END) )
    {
      CFStart = 1;
    }

    if ( CFStart )
    {
      if ( !startWith(tmp_buf, TAG_CF_START/*"L000"*/) &&
           !startWith(tmp_buf, TAG_EBR_START) &&
           !startWith(tmp_buf, TAG_FIRST_FUSE_ADDR) &&
           !startWith(tmp_buf, TAG_END) &&
           strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          // printf("++\n");
          (*cf_size)++;
        }
        else
        {
           CFStart = 0;
        }
      }
    }
    else if ( UFMStart )
    {
      if ( !startWith(tmp_buf, TAG_UFM/*"NOTE TAG DATA"*/) && !startWith(tmp_buf, "L") && strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          (*ufm_size)++;
        }
        else
        {
          UFMStart = 0;
        }
      }
    }
  }

  //cf must greater than 0
  if ( !(*cf_size) )
  {
    ret = -1;
  }
  printf("cf_size: %d  ufm_size: %d\n", *cf_size, *ufm_size);

  return ret;
}

int
NX_JED_File_Parser(FILE *jed_fd, CPLDInfo *dev_info, int cf_size, int ufm_size)
{
  /**TAG Information**/
  const char TAG_QF[] = "QF";
  const char TAG_CF_START[] = "L000";
  const char TAG_UFM[] = "NOTE TAG DATA";
  const char TAG_ROW[] = "NOTE FEATURE";
  const char TAG_CHECKSUM[] = "C";
  const char TAG_USERCODE[] = "NOTE User Electronic";
  const char TAG_EBR_START[] = "NOTE EBR_INIT DATA";
  const char TAG_FIRST_FUSE_ADDR[] = "L";
  const char TAG_END[] = "NOTE END CONFIG DATA";
  /**TAG Information**/

  int ReadLineSize = LATTICE_COL_SIZE + 3;//the len of 128 only contain data size, '\n' need to be considered, too.
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
  int cf_size_used = (cf_size * LATTICE_COL_SIZE) / 8; // unit: bytes
  int ufm_size_used = (ufm_size * LATTICE_COL_SIZE) / 8; // unit: bytes

  dev_info->CF = (unsigned int*)malloc( cf_size_used );
  memset(dev_info->CF, 0, cf_size_used);

  if ( ufm_size_used )
  {
    dev_info->UFM = (unsigned int*)malloc( ufm_size_used );
    memset(dev_info->UFM, 0, ufm_size_used);
  }

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
      printf("[QF]%ld\n",(long) dev_info->QF);
#endif
    }
    else if ( startWith(tmp_buf, TAG_CF_START/*"L000"*/) )
    {
#ifdef CPLD_DEBUG
      printf("[CFStart]\n");
#endif
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_EBR_START) )
    {
#ifdef CPLD_DEBUG
      printf("[CFStart]\n");
#endif
      CFStart = 1;
    }
    else if ( startWith(tmp_buf, TAG_END) )
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
#ifdef VERBOSE_DEBUG
      printf("[%s][%d][%c]", __func__, (int) strlen(tmp_buf), tmp_buf[0]);
#endif
      if ( !startWith(tmp_buf, TAG_CF_START/*"L000"*/) &&
           !startWith(tmp_buf, TAG_EBR_START) &&
           !startWith(tmp_buf, TAG_FIRST_FUSE_ADDR) &&
           !startWith(tmp_buf, TAG_END) &&
           strlen(tmp_buf) != 1 )
      {
        if ( startWith(tmp_buf,"0") || startWith(tmp_buf,"1") )
        {
          current_addr = (dev_info->CF_Line * LATTICE_COL_SIZE) / 32;

          memset(data_buf, 0, sizeof(data_buf));

          memcpy(data_buf, tmp_buf, LATTICE_COL_SIZE);

          /*convert string to byte data*/
          ShiftData(data_buf, &dev_info->CF[current_addr], LATTICE_COL_SIZE);
#ifdef VERBOSE_DEBUG
          printf("[%d]\t%08x %08x %08x %08x\n",(int)dev_info->CF_Line, dev_info->CF[current_addr],dev_info->CF[current_addr+1],dev_info->CF[current_addr+2],dev_info->CF[current_addr+3]);
#endif
          //each data has 128bits(4*unsigned int), so the for-loop need to be run 4 times
          for ( i = 0; i < sizeof(unsigned int); i++ )
          {
            JED_CheckSum += (dev_info->CF[current_addr+i]>>24) & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i]>>16) & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i]>>8)  & 0xff;
            JED_CheckSum += (dev_info->CF[current_addr+i])     & 0xff;
          }

            dev_info->CF_Line++;
        }
        else
        {
#ifdef CPLD_DEBUG
          printf("[%s]CF Line: %d\n", __func__, (int)dev_info->CF_Line);
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
printf("[UserCode]%x\n",dev_info->Version);
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
#ifdef VERBOSE_DEBUG
          printf("%x %x %x %x\n",dev_info->UFM[current_addr],dev_info->UFM[current_addr+1],dev_info->UFM[current_addr+2],dev_info->UFM[current_addr+3]);
#endif
          dev_info->UFM_Line++;
        }
        else
        {
#ifdef CPLD_DEBUG
          printf("[%s]UFM Line: %d\n", __func__, (int) dev_info->UFM_Line);
#endif
          UFMStart = 0;
        }
      }
    }
  }

  printf("CheckSum from jed: %04X, Caculated CheckSum: %04X \n", dev_info->CheckSum, JED_CheckSum&0xffff);
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

int get_lattice_dev_list(struct cpld_dev_info** list )
{
  int size = sizeof(lattice_dev_list)/ sizeof(lattice_dev_list[0]);
  *list = lattice_dev_list;
  return size;
}
