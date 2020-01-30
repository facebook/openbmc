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
      ast_jtag_run_test_idle(0, 0, 3);
      do
      {
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_CHECK_BUSY);
	usleep(1000);

	buf[0] = 0x0;

	ast_jtag_tdo_xfer(0, 8, &buf[0]);

	buf[0] = (buf[0] >> 7) & 0x1;

	if (buf[0] == 0x0)
	{
	  break;
	}
	RETRY--;

      } while ( RETRY );
      break;

    case CHECK_STATUS:
      ast_jtag_run_test_idle( 0, 0, 3);
      do
      {
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_READ_STATUS);
	usleep(1000);

	buf[0] = 0x0;

	ast_jtag_tdo_xfer(0, 32, &buf[0]);

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
    ast_jtag_run_test_idle(0, 0, 3);

    //set page to program page
    ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, LCMXO2_LSC_PROG_INCR_NV);

    //send data
    ast_jtag_tdi_xfer(0, LATTICE_COL_SIZE, &dev_info->CF[CurrentAddr]);

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
    ast_jtag_run_test_idle(0, 0, 3);

    //set page to program page
    ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, LCMXO2_LSC_PROG_INCR_NV);

    //send data
    ast_jtag_tdi_xfer(0, LATTICE_COL_SIZE, &dev_info->UFM[CurrentAddr]);

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

  ast_jtag_run_test_idle(0, 0, 3);
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDRESS);

  buff[0] = 0x04;
  ast_jtag_tdi_xfer(0, LATTICE_INS_LENGTH, &buff[0]);
  usleep(1000);

  ast_jtag_run_test_idle(0, 0, 3);
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_READ_INCR_NV);
  usleep(1000);

#ifdef CPLD_DEBUG
  printf("[%s] dev_info->CF_Line: %d\n", __func__, dev_info->CF_Line);
#endif

  for(i = 0; i < dev_info->CF_Line; i++)
  {
    printf("Verify Data: %d/%d (%.2f%%) \r",(i+1), dev_info->CF_Line, (((i+1)/(float)dev_info->CF_Line)*100));

    current_addr = (i * LATTICE_COL_SIZE) / 32;

    memset(buff, 0, sizeof(buff));

    ast_jtag_tdo_xfer(0, LATTICE_COL_SIZE, buff);

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

  ast_jtag_run_test_idle(0, 0, 3);
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_ISC_ENABLE_X);
  dr_data[0] = 0x08;
  ast_jtag_tdi_xfer(0, LATTICE_INS_LENGTH, dr_data);

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

  ast_jtag_run_test_idle(0, 0, 3);
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_ISC_PROGRAM_DONE);

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
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_ISC_DISABLE);

  return ret;
}

static int
LCMXO2Family_cpld_Check_ID()
{
  unsigned int dr_data[4] = {0};
  int ret = -1;
  int i;

  //RUNTEST IDLE
  ast_jtag_run_test_idle(0, 0, 3);

#ifdef CPLD_DEBUG
  printf("[%s] RUNTEST IDLE\n", __func__);
#endif

  //Check the IDCODE_PUB
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
  dr_data[0] = 0x0;
  ast_jtag_tdo_xfer(0, 32, dr_data);

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

  ast_jtag_run_test_idle(0, 0, 3);
  //Check the IDCODE_PUB
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
  dr_data[0] = 0x0;
  ast_jtag_tdo_xfer( 0, 32, dr_data);

  *dev_id = dr_data[0];

  return 0;
}

static int
LCMXO2Family_cpld_Erase(int erase_type)
{
  unsigned int dr_data[4] = {0};
  int ret = 0;

  ast_jtag_run_test_idle(0, 0, 3);
  //Erase the Flash
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_ISC_ERASE);

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

  ast_jtag_tdi_xfer(0, LATTICE_INS_LENGTH, dr_data);

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
  ast_jtag_run_test_idle(0, 0, 3);
  //Shift in LSC_INIT_ADDRESS(0x46) instruction
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDRESS);

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
    ast_jtag_run_test_idle(0, 0, 3);
    //program UFM
    ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_LSC_INIT_ADDR_UFM);

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
  ast_jtag_tdi_xfer(0, 32, dr_data);

#ifdef CPLD_DEBUG
  printf("[%s] Write USERCODE: %x\n", __func__, dr_data[0]);
#endif

  ast_jtag_run_test_idle(0, 0, 3);
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_ISC_PROGRAM_USERCOD);
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
  ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_USERCODE);

#ifdef CPLD_DEBUG
  printf("[%s] READ USERCODE(0xC0)\n", __func__);
#endif

  //Read UserCode
  dr_data[0] = 0;
  ast_jtag_tdo_xfer(0, 32, dr_data);
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
#if 0
/*
 * TODO: Not yet tested at all, need to verify the JED and
 * change the function accordingly. This function needs
 * to be changed (minor) to handle all the cases
 */
int lcmxo2_2000hc_cpld_program(FILE *jed_fd)
{
	int i;
	unsigned int *dr_data;
	//file
	unsigned int *jed_data;

	unsigned int row  = 0;

	dr_data = malloc(((424/32) + 1) * sizeof(unsigned int));
	jed_data = malloc(((cur_dev->dr_bits/32) + 1) * sizeof(unsigned int));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	if(dr_data[0] != 0x12BB043) {
#ifdef VERBOSE_DEBUG
			printf("ID Fail : %08x [0x012BB043] \n",dr_data[0]);
#endif

		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR	424 TDI (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, (424/32 + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer( 0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8 TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8 TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x00;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8 TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	dr_data[0] = 0x01;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8 TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8 TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8 TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x08;
	ast_jtag_tdi_xfer( 0, 8, dr_data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Erase the Flash
	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8 TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0E;
	ast_jtag_tdi_xfer( 0, 8, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8 TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1 TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	dr_data[0] = 0;
	for(i = 0; i <800 ; i++) {
		usleep(2000);
		ast_jtag_tdo_xfer( 0, 1, dr_data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
	if(dr_data[0] != 0x0) {
#ifdef CPLD_DEBUG
			printf("%x [0x0]\n", dr_data[0]);
#endif
	}

#ifdef CPLD_DEBUG
		printf("Erase Done \n");
#endif

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_parse_header(jed_fd);

	//! Program CFG

#ifdef CPLD_DEBUG
		printf("Program CFG \n");
#endif

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	dr_data[0] = 0x04;
	ast_jtag_tdo_xfer(0, 32, dr_data);

#ifdef CPLD_DEBUG
		printf("Program 3198 .. \n");
#endif

//	mode = SW_MODE;
	for(row =0 ; row < cur_dev->row_num; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits/32) * sizeof(unsigned int));
		jed_file_parser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8 TDI  (70);

		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0x70);


		//! Shift in Data Row = 1
		//SDR 128 TDI  (120600000040000000DCFFFFCDBDFFFF);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);
		usleep(1000);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8 TDI  (F0);

		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0xF0);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1 TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for(i = 0;i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if(dr_data[0] == 0) break;
		}

		if(dr_data[0] != 0) {
#ifdef CPLD_DEBUG
				printf("row %d, Fail [%d] \n", row, dr_data[0]);
#endif
		} else {
#ifdef CPLD_DEBUG
				printf(".");
#endif
		}

	}
//	mode = HW_MODE;

	//! Program the UFM
#ifdef CPLD_DEBUG
		printf("Program the UFM : 640\n");
#endif

	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR 8	TDI  (47);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x47);

	for(row =0 ; row < 640; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits/32) * sizeof(unsigned int));
		jed_file_parser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8	TDI  (70);
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0x70);

		//! Shift in Data Row = 1
		//SDR 128 TDI  (00000000000000000000000000000000);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8	TDI  (F0);
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, 0xF0);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1	TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for(i = 0;i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if(dr_data[0] == 0) break;
		}

		if(dr_data[0] != 0) {
#ifdef CPLD_DEBUG
				printf("row %d, Fail [%d] \n",row, dr_data[0]);
#endif
		} else {
#ifdef CPLD_DEBUG
				printf(".");
#endif
		}

	}

	//! Program USERCODE

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);

	//SDR 32	TDI  (00000000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdi_xfer(0, 32, dr_data);

	//! Shift in ISC PROGRAM USERCODE(0xC2) instruction
	//SIR 8	TDI  (C2);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC2);
	usleep(2000);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Program Feature Rows

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (02);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x02;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in LSC_PROG_FEATURE(0xE4) instruction
	//SIR 8	TDI  (E4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE4);

	//SDR 64	TDI  (0000000000000000);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdi_xfer(0, 64, dr_data);


	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE7);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer(0, 64, dr_data);

	//! Shift in in LSC_PROG_FEABITS(0xF8) instruction
	//SIR 8	TDI  (F8);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF8);

	//SDR 16	TDI  (0620);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0620;
	ast_jtag_tdi_xfer(0, 16, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFB);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x0;
	ast_jtag_tdo_xfer(0, 16, dr_data);

	//! Program DONE bit

	//! Shift in ISC PROGRAM DONE(0x5E) instruction
	//SIR 8	TDI  (5E);
	//RUNTEST IDLE	2 TCK;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x5E);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for(i = 0;i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (C4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFF);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)//;
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFF);

	free(jed_data);
	free(dr_data);

	return 0;

}

int lcmxo2_2000hc_cpld_flash_enable(void)
{
	unsigned int data=0;
	unsigned int *dr_data;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
		return -1;;
	}

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);
	usleep(3000);

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR 424 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((424/32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X(0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (00)
	//	MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	if (dr_data != NULL)
		free(dr_data);

	return 0;
}

int lcmxo2_2000hc_cpld_flash_disable(void)
{
	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);
	usleep(1000);

	return 0;
}

int lcmxo2_2000hc_cpld_ver(unsigned int *ver)
{
	unsigned int *dr_data;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
		return -1;;
	}

	//! Verify USERCODE
	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
	*ver = dr_data[0];

	if (dr_data != NULL)
		free(dr_data);

	return 0;
}

/*
 * TODO: Tested with CPLD programmed against F08_V01.jed
 * It needs to be tested thoroughly before using it.
 */
int lcmxo2_2000hc_cpld_verify(FILE *jed_fd)
{
	int i;
	unsigned int data=0;
	unsigned int *jed_data = NULL;
	unsigned int *dr_data = NULL;
	unsigned int row  = 0;
	int cmp_err = 0;
	int parser_err = 0;

	int cfg_err = 0;
	int ufm_err = 0;

	int ret_err = -1;

	unsigned int alloc_size = ((cur_dev->dr_bits /32 + 1) * sizeof(unsigned int) + 4096 ) % 4096;

	dr_data = (unsigned int *)malloc(alloc_size);
	if (dr_data == NULL) {
#ifdef CPLD_DEBUG
			printf("memory allocation for dr_data failed.\n");
#endif

		goto err_handle;
	}

	jed_data = (unsigned int *)malloc(alloc_size);
	if (jed_data == NULL) {
#ifdef CPLD_DEBUG
			printf("memory allocation for jed_data failed.\n");
#endif

		goto err_handle;
	}

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);
	usleep(3000);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (012BB043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	if(dr_data[0] != 0x12BB043) {
#ifdef CPLD_DEBUG
			printf("ID Fail : %08x [0x012BB043] \n",dr_data[0]);
#endif

		free(dr_data);
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x1C);

	//SDR 424 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((424/32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 424, dr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (00)
	//	MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Verify USERCODE
#ifdef CPLD_DEBUG
		printf("Verify USERCODE \n");
#endif

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

#ifdef CPLD_DEBUG
		printf("CPLD USERCODE = 0x%x\n", dr_data[0]);
#endif


	//! Verify the Flash
#ifdef CPLD_DEBUG
		printf("Starting to Verify Device . . . This will take a few seconds\n");
#endif

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x46);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x04;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR 8	TDI  (73);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x73);
	usleep(3000);

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_parse_header(jed_fd);

#ifdef CPLD_DEBUG
		printf("Verify CONFIG 3198 \n");
#endif
	cmp_err = 0;
	row = 0;

	//for(row = 0; row < cur_dev->row_num; row++) {
	for(row = 0; row < 3198; row++) {
		memset(dr_data, 0, 16);
		memset(jed_data, 0, 16);
		parser_err = jed_file_parser(jed_fd, 128, jed_data);
		if (parser_err == -1) {
			jed_file_parse_header(jed_fd);
			parser_err = jed_file_parser(jed_fd, 128, jed_data);
			if (parser_err == -1) {
				goto err_handle;
			}
		}

		ast_jtag_tdo_xfer( 0, 128, dr_data);

		for(i = 0; i < 4; i++) {
			if(dr_data[i] != jed_data[i]) {
				cmp_err = 1;
				goto err_handle;
			}
		}

		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
		usleep(3000);
	}
	cfg_err = 0;

	jed_file_parse_header(jed_fd);

	//! Verify the UFM

	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR	8	TDI  (47);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x47);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR	8	TDI  (73);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x73);
	usleep(3000);

#ifdef CPLD_DEBUG
		printf("Verify the UFM 640 \n");
#endif
	//! Shift out Data Row
	for(row =0 ; row < 640; row++) {
		memset(dr_data, 0, 16);
		memset(jed_data, 0, 16);
		parser_err = jed_file_parser(jed_fd, 128, jed_data);
		if (parser_err == -1) {
			jed_file_parse_header(jed_fd);
			parser_err = jed_file_parser(jed_fd, 128, jed_data);
			if (parser_err == -1) {
				goto err_handle;
			}
		}

		ast_jtag_tdo_xfer( 0, 128, dr_data);

		//for(i = 0; i < (cur_dev->dr_bits /32); i++) {
		for(i = 0; i < 4; i++) {
			if(dr_data[i] != jed_data[i]) {
				cmp_err = 1;
				goto err_handle;
			}
		}
		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
		usleep(3000);
	}
	ufm_err = 0;

	//! Verify USERCODE
#ifdef CPLD_DEBUG
		printf("Verify USERCODE \n");
#endif

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xC0);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);


	//! Verify Feature Rows

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, dr_data);

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xE7);
	usleep(3000);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 64, dr_data);

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xFB);
	usleep(3000);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer( 0, 16, dr_data);
#ifdef CPLD_DEBUG
		printf("read %x [0x0620] \n", dr_data[0] & 0xffff);
#endif

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Verify Done Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//	TDO  (04)
	//	MASK (C4);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);
	usleep(1000);

	//! Verify SRAM DONE Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (84);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	if (!cfg_err && !ufm_err)
		ret_err = 0;

err_handle:
	if (dr_data != NULL)
		free(dr_data);

	if (jed_data != NULL)
		free(jed_data);

	if(cmp_err) {
#ifdef CPLD_DEBUG
			printf("Verify Error !!\n");
#endif
	} else {
#ifdef CPLD_DEBUG
			printf("Verify Done !!\n");
#endif
	}

	if (parser_err) {
#ifdef CPLD_DEBUG
#endif
			printf("Error while parsing JED file\n");
	} else {
#ifdef CPLD_DEBUG
			printf("NO Error while parsing JED file\n");
#endif
	}

	return ret_err;

}

/*
 * TODO: Not tested at all, needs to be changed to handle
 * CPLD LCMXO2-200HC
 */
int lcmxo2_2000hc_cpld_erase(void)
{
	int i = 0;
	unsigned int *sdr_data;
	unsigned int data=0;
	unsigned int sdr_array = 0;

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle( 0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	data = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, &data);

	if(data != 0x12BB043) {
#ifdef CPLD_DEBUG
			printf("ID Fail : %08x [0x012BB043] \n",data);
#endif
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, PRELOAD);

	//SDR 424 TDI (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	sdr_array = 424/32 + 1;
	sdr_data = malloc(sdr_array * sizeof(unsigned int));
	memset(sdr_data, 0xff, sdr_array * sizeof(unsigned int));
	ast_jtag_tdi_xfer( 0, 424, sdr_data);
	free(sdr_data);

	//! Enable the Flash (Transparent Mode)
	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	//! Shift in ISC ENABLE X (0x74) instruction
	//SIR 8	TDI  (74);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x74);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer( 0, 8, &data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	data = 0x00000000;
	ast_jtag_tdo_xfer( 0, 32, &data);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Erase the Flash

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x0E);

	//SDR 8	TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	data = 0x0E;
	ast_jtag_tdi_xfer( 0, 8, &data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0xF0);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	data = 0;
	for(i = 0; i <800 ; i++) {
		usleep(2000);
		ast_jtag_tdo_xfer( 0, 1, &data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x3C);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, 0x26);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, BYPASS);

	usleep(1000);

	return 0;

}
#endif
