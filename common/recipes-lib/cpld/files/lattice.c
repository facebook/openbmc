#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openbmc/ast-jtag.h>
#include "cpld.h"
#include "lattice.h"

#define MAX_RETRY 4000
#define LATTICE_COL_SIZE 128
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

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

//#define CPLD_DEBUG //enable debug message
//#define VERBOSE_DEBUG //enable detail debug message

enum
{
  CHECK_BUSY = 0,
  CHECK_STATUS = 1,
};

enum
{
  Only_CF = 0,
  Both_CF_UFM = 1,
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

static unsigned int
LCMXO2Family_Check_Device_Status(int mode)
{
  int RETRY = MAX_RETRY;
  unsigned int buf[4] = {0};

  switch (mode)
  {
    case CHECK_BUSY:
//      ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
      do
      {
	ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_CHECK_BUSY);
	usleep(1000);

	buf[0] = 0x0;

	ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 8, &buf[0]);

	buf[0] = (buf[0] >> 7) & 0x1;

	if (buf[0] == 0x0)
	{
	  break;
	}
	RETRY--;

      } while ( RETRY );
      break;

    case CHECK_STATUS:
//      ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
      do
      {
	ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_READ_STATUS);
	usleep(1000);

	buf[0] = 0x0;

	ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, &buf[0]);

	buf[0] = (buf[0] >> 12) & 0x3;

	if (buf[0] == 0x0)
	{
	  break;
	}
	RETRY--;
      } while ( RETRY );
      break;

    default:
      break;

  }

  return buf[0];
}

/*write cf data*/
static int
LCMXO2Family_SendCFdata(CPLDInfo *dev_info)
{
  int ret = 0;
  int CurrentAddr = 0;
  int i;
  unsigned status;

  for (i = 0; i < dev_info->CF_Line; i++)
  {
    printf("Writing Data: %d/%d (%.2f%%) \r",(i+1), dev_info->CF_Line, (((i+1)/(float)dev_info->CF_Line)*100));

    CurrentAddr = (i * LATTICE_COL_SIZE) / 32;

    //RUNTEST    IDLE    15 TCK    1.00E-003 SEC;
//    ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);

    //set page to program page
    ast_jtag_sir_xfer(JTAG_STATE_PAUSEIR, LATTICE_INS_LENGTH, LCMXO2_LSC_PROG_INCR_NV);

    //send data
    ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, LATTICE_COL_SIZE, &dev_info->CF[CurrentAddr]);

    //usleep(1000);

    status = LCMXO2Family_Check_Device_Status(CHECK_BUSY);
    if (status != 0)
    {
      printf("[%s]Write CF Error, status = %x\n", __func__, status);
      ret = -1;
      break;
    }
  }

  printf("\n");

  return ret;
}

/*write ufm data if need*/
static int
LCMXO2Family_SendUFMdata(CPLDInfo *dev_info)
{
  int ret = 0;
  int CurrentAddr = 0;
  int i;
  unsigned int status;

  for (i = 0; i < dev_info->UFM_Line; i++)
  {
    CurrentAddr = (i * LATTICE_COL_SIZE) / 32;

    //RUNTEST    IDLE    15 TCK    1.00E-003 SEC;
//    ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);

    //set page to program page
    ast_jtag_sir_xfer(JTAG_STATE_PAUSEIR, LATTICE_INS_LENGTH, LCMXO2_LSC_PROG_INCR_NV);

    //send data
    ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, LATTICE_COL_SIZE, &dev_info->UFM[CurrentAddr]);

    //usleep(1000);

    status = LCMXO2Family_Check_Device_Status(CHECK_BUSY);
    if (status != 0)
    {
      printf("[%s]Write UFM Error, status = %x\n", __func__, status);
      ret = -1;
      break;
    }

  }

  return ret;
}

/*check the size of cf and ufm*/
static int
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

static int
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
#ifdef VERBOSE_DEBUG
      printf("[%s][%d][%c]", __func__, strlen(tmp_buf), tmp_buf[0]);
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
          printf("[%d]%x %x %x %x\n",dev_info->CF_Line, dev_info->CF[current_addr],dev_info->CF[current_addr+1],dev_info->CF[current_addr+2],dev_info->CF[current_addr+3]);
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
#ifdef VERBOSE_DEBUG
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

static int
LCMXO2Family_cpld_verify(CPLDInfo *dev_info)
{
  int i;
  int result;
  int current_addr = 0;
  unsigned int buff[4] = {0};
  int ret = 0;

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDRESS);

  buff[0] = 0x04;
  ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, &buff[0]);
  usleep(1000);

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_READ_INCR_NV);
  usleep(1000);

#ifdef CPLD_DEBUG
  printf("[%s] dev_info->CF_Line: %d\n", __func__, dev_info->CF_Line);
#endif

  for(i = 0; i < dev_info->CF_Line; i++)
  {
    printf("Verify Data: %d/%d (%.2f%%) \r",(i+1), dev_info->CF_Line, (((i+1)/(float)dev_info->CF_Line)*100));

    current_addr = (i * LATTICE_COL_SIZE) / 32;

    memset(buff, 0, sizeof(buff));

    ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, LATTICE_COL_SIZE, buff);

    result = memcmp(buff, &dev_info->CF[current_addr], sizeof(unsigned int));

    if (result)
    {

#ifdef CPLD_DEBUG
      printf("\nPage#%d (%x %x %x %x) did not match with CF (%x %x %x %x)\n",
	     i, buff[0], buff[1], buff[2], buff[3],
	     dev_info->CF[current_addr], dev_info->CF[current_addr+1],
	     dev_info->CF[current_addr+2], dev_info->CF[current_addr+3]);
#endif
      ret = -1;
      break;
    }
  }

  printf("\n");

  if (-1 == ret)
  {
    printf("\n[%s] Verify CPLD FW Error\n", __func__);
  }
#ifdef CPLD_DEBUG
  else
  {
    printf("\n[%s] Verify CPLD FW Pass\n", __func__);
  }
#endif

  return ret;
}

static int
LCMXO2Family_cpld_Start()
{
  unsigned int dr_data[4] = {0};
  int ret = 0;

  //Enable the Flash (Transparent Mode)
#ifdef CPLD_DEBUG
  printf("[%s] Enter transparent mode!\n", __func__);
#endif

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_ISC_ENABLE_X);
  dr_data[0] = 0x08;
  ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, dr_data);

  //LSC_CHECK_BUSY(0xF0) instruction
  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_BUSY);

  if (dr_data[0] != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, dr_data[0]);
    ret = -1;
  }
  else
  {
    ret = 0;
  }

#ifdef CPLD_DEBUG
  printf("[%s] READ_STATUS(0x3C)!\n", __func__);
#endif
  //READ_STATUS(0x3C) instruction
  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_STATUS);

  if (dr_data[0] != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, dr_data[0]);
    ret = -1;
  }
  else
  {
    ret = 0;
  }

  return ret;
}

static int
LCMXO2Family_cpld_End()
{
  int ret = 0;
  unsigned int status;

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_ISC_PROGRAM_DONE);

#ifdef CPLD_DEBUG
  printf("[%s] Program DONE bit\n", __func__);
#endif

  //Read CHECK_BUSY

  status = LCMXO2Family_Check_Device_Status(CHECK_BUSY);
  if (status != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, status);
    ret = -1;
  }

#ifdef CPLD_DEBUG
  printf("[%s] READ_STATUS: %x\n", __func__, ret);
#endif

  status = LCMXO2Family_Check_Device_Status(CHECK_STATUS);
  if (status != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, status);
    ret = -1;
  }

#ifdef CPLD_DEBUG
  printf("[%s] READ_STATUS: %x\n", __func__, ret);
#endif

  //Exit the programming mode
  //Shift in ISC DISABLE(0x26) instruction
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_ISC_DISABLE);

  return ret;
}

static int
LCMXO2Family_cpld_Check_ID()
{
  unsigned int dr_data[4] = {0};
  int ret = -1;
  int i;

  //RUNTEST IDLE
  ast_jtag_run_test_idle(1, JTAG_STATE_TLRESET, 3);

#ifdef CPLD_DEBUG
  printf("[%s] RUNTEST IDLE\n", __func__);
#endif

  //Check the IDCODE_PUB
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
  dr_data[0] = 0x0;
  ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, dr_data);

#ifdef CPLD_DEBUG
  printf("[%s] ID Code: %x\n", __func__, dr_data[0]);
#endif

  for (i = 0; i < ARRAY_SIZE(lattice_dev_list); i++)
  {
    if (dr_data[0] == lattice_dev_list[i].dev_id)
    {
      ret = 0;
      break;
    }
  }

  return ret;
}

int
LCMXO2Family_cpld_Get_id(unsigned int *dev_id)
{
  unsigned int dr_data[4] = {0};

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  //Check the IDCODE_PUB
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
  dr_data[0] = 0x0;
  ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, dr_data);

  *dev_id = dr_data[0];

  return 0;
}

static int
LCMXO2Family_cpld_Erase(int erase_type)
{
  unsigned int dr_data[4] = {0};
  int ret = 0;

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  //Erase the Flash
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_ISC_ERASE);

#ifdef CPLD_DEBUG
  printf("[%s] ERASE(0x0E)!\n", __func__);
#endif

  switch (erase_type)
  {
    case Only_CF:
      dr_data[0] = 0x04; //0x4=CF
      break;

    case Both_CF_UFM:
      dr_data[0] = 0x0C; //0xC=CF and UFM
      break;
  }

  ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, dr_data);

  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_BUSY);

  if (dr_data[0] != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, dr_data[0]);
    ret = -1;
  }
  else
  {
    ret = 0;
  }

#ifdef CPLD_DEBUG
  //Shift in LSC_READ_STATUS(0x3C) instruction
  printf("[%s] READ_STATUS!\n", __func__);
#endif

  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_STATUS);

  if (dr_data[0] != 0)
  {
    printf("Erase Failed, status = %x\n", dr_data[0]);
    ret = -1;
  }
  else
  {
    ret = 0;
  }

#ifdef CPLD_DEBUG
  printf("[%s] Erase Done!\n", __func__);
#endif

  return ret;
}

static int
LCMXO2Family_cpld_program(CPLDInfo *dev_info)
{
  int ret = 0;
  unsigned int dr_data[4] = {0};

#ifdef CPLD_DEBUG
  //Program CFG
  printf("[%s] Program CFG \n", __func__);
#endif
//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  //Shift in LSC_INIT_ADDRESS(0x46) instruction
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDRESS);

#ifdef CPLD_DEBUG
  printf("[%s] INIT_ADDRESS(0x46) \n", __func__);
#endif

  ret = LCMXO2Family_SendCFdata(dev_info);
  if (ret < 0)
  {
    goto error_exit;
  }

  if (dev_info->UFM_Line)
  {
//    ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
    //program UFM
    ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDR_UFM);

    ret = LCMXO2Family_SendUFMdata(dev_info);
    if (ret < 0)
    {
      goto error_exit;
    }
  }

#ifdef CPLD_DEBUG
  printf("[%s] Update CPLD done \n", __func__);
#endif

  //Read the status bit
  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_STATUS);

  if (dr_data[0] != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, dr_data[0]);
    ret = -1;
    goto error_exit;
  }

#ifdef CPLD_DEBUG
  //Program USERCODE
  printf("[%s] Program USERCODE\n", __func__);
#endif

  //Write UserCode
  dr_data[0] = dev_info->Version;
  ast_jtag_tdi_xfer(JTAG_STATE_TLRESET, 32, dr_data);

#ifdef CPLD_DEBUG
  printf("[%s] Write USERCODE: %x\n", __func__, dr_data[0]);
#endif

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_ISC_PROGRAM_USERCOD);
  usleep(2000);

#ifdef CPLD_DEBUG
  printf("[%s] PROGRAM USERCODE(0xC2)\n", __func__);
#endif

  //Read the status bit
  dr_data[0] = LCMXO2Family_Check_Device_Status(CHECK_STATUS);

  if (dr_data[0] != 0)
  {
    printf("[%s] Device Busy, status = %x\n", __func__, dr_data[0]);
    ret = -1;
    goto error_exit;
  }

#ifdef CPLD_DEBUG
  printf("[%s] READ_STATUS: %x\n", __func__, dr_data[0]);
#endif

error_exit:
  return ret;
}

int
LCMXO2Family_cpld_Get_Ver(unsigned int *ver)
{
  int ret;
  unsigned int dr_data[4]={0};

  ret = LCMXO2Family_cpld_Check_ID();

  if (ret < 0)
  {
    printf("[%s] Unknown Device ID!\n", __func__);
    goto error_exit;
  }

  ret = LCMXO2Family_cpld_Start();
  if (ret < 0)
  {
    printf("[%s] Enter Transparent mode Error!\n", __func__);
    goto error_exit;
  }

  //Shift in READ USERCODE(0xC0) instruction;
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_USERCODE);

#ifdef CPLD_DEBUG
  printf("[%s] READ USERCODE(0xC0)\n", __func__);
#endif

  //Read UserCode
  dr_data[0] = 0;
  ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, dr_data);
  *ver = dr_data[0];

  ret = LCMXO2Family_cpld_End();
  if (ret < 0)
  {
    printf("[%s] Exit Transparent Mode Failed!\n", __func__);
  }

error_exit:
  return ret;
}

int
LCMXO2Family_cpld_update(FILE *jed_fd, char* key, char is_signed)
{
  CPLDInfo dev_info = {0};
  int cf_size = 0;
  int ufm_size = 0;
  int erase_type = 0;
  int ret;

#ifdef CPLD_DEBUG
  printf("[%s]\n",__func__);
#endif

  ret = LCMXO2Family_cpld_Check_ID();
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

  ret = LCMXO2Family_cpld_Start();
  if ( ret < 0 )
  {
    printf("[%s] Enter Transparent mode Error!\n", __func__);
    goto error_exit;
  }

  if ( dev_info.UFM_Line )
  {
    erase_type = Both_CF_UFM;
  }
  else
  {
    erase_type = Only_CF;
  }

  ret = LCMXO2Family_cpld_Erase(erase_type);
  if ( ret < 0 )
  {
    printf("[%s] Erase failed!\n", __func__);
    goto error_exit;
  }

  ret = LCMXO2Family_cpld_program(&dev_info);
  if ( ret < 0 )
  {
    printf("[%s] Program failed!\n", __func__);
    goto error_exit;
  }

  ret = LCMXO2Family_cpld_verify(&dev_info);
  if ( ret < 0 )
  {
    printf("[%s] Verify Failed!\n", __func__);
    goto error_exit;
  }

  ret = LCMXO2Family_cpld_End();
  if ( ret < 0 )
  {
    printf("[%s] Exit Transparent Mode Failed!\n", __func__);
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

static int cpld_dev_open(cpld_intf_t intf, uint8_t id, void *attr)
{
  if (intf == INTF_JTAG) {
    ast_jtag_set_mode(JTAG_XFER_HW_MODE);
    return ast_jtag_open();
  } else {
    printf("[%s] Interface type %d is not supported\n", __func__, intf);
    return -1;
  }
}

static int cpld_dev_close(cpld_intf_t intf)
{
  if (intf == INTF_JTAG) {
    ast_jtag_close();
  } else {
    printf("[%s] Interface type %d is not supported\n", __func__, intf);
  }

  return 0;
}

struct cpld_dev_info lattice_dev_list[] = {
  [0] = {
    .name = "LCMXO2-2000HC",
    .dev_id = 0x012BB043,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = LCMXO2Family_cpld_Get_Ver,
    .cpld_program = LCMXO2Family_cpld_update,
    .cpld_dev_id = LCMXO2Family_cpld_Get_id,
  },
  [1] = {
    .name = "LCMXO2-4000HC",
    .dev_id = 0x012BC043,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = LCMXO2Family_cpld_Get_Ver,
    .cpld_program = LCMXO2Family_cpld_update,
    .cpld_dev_id = LCMXO2Family_cpld_Get_id,
  },
  [2] = {
    .name = "LCMXO2-7000HC",
    .dev_id = 0x012BD043,
    .cpld_open = cpld_dev_open,
    .cpld_close = cpld_dev_close,
    .cpld_ver = LCMXO2Family_cpld_Get_Ver,
    .cpld_program = LCMXO2Family_cpld_update,
    .cpld_dev_id = LCMXO2Family_cpld_Get_id,
  }
};

/*************************************************************************************/
void jed_file_parse_header(FILE *jed_fd)
{
	//File
	char tmp_buf[160];

	//Header paser
	while(fgets(tmp_buf, 120, jed_fd)!= NULL) {
#ifdef VERBOSE_DEBUG
			printf("%s \n",tmp_buf);
#endif
		if (tmp_buf[0] == 0x4C) { // "L"
			break;
		}
	}
}

unsigned int jed_file_parser(FILE *jed_fd, unsigned int len, unsigned int *dr_data)
{
	int i = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;

	//Jed row
	for(i = 0; i < len; i++) {
		input_char = fgetc(jed_fd);
		if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
			if (input_char == 0x30) {
				input_bit = 0;
			} else {
				input_bit = 1;
			}
#ifdef VERBOSE_DEBUG
				printf("%d",input_bit);
#endif
			dr_data[sdr_array] |= (input_bit << data_bit);
			data_bit++;
			bit_cnt++;

			if((data_bit % 32) == 0) {
#ifdef VERBOSE_DEBUG
					printf(" [%i] : %x \n",sdr_array, dr_data[sdr_array]);
#endif
				data_bit = 0;
				sdr_array++;
			}
		} else if (input_char == 0xd) {
			i--;
		} else if (input_char == 0xa) {
			i--;
		} else {
#ifdef VERBOSE_DEBUG
				printf("parser errorxx [%x]\n", input_char);
#endif
			break;
		}
	}
#ifdef VERBOSE_DEBUG
		printf(" [%i] : %x , Total %d \n",sdr_array, dr_data[sdr_array], bit_cnt);
#endif

	if(bit_cnt != len) {
#ifdef VERBOSE_DEBUG
			printf("File Error \n");
#endif

		return -1;
	}

	return 0;
}
