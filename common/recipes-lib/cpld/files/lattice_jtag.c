#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <openbmc/ast-jtag.h>
#include "cpld.h"
#include "lattice_jtag.h"
#include "lattice.h"

#define MAX_RETRY 4000

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
    printf("Writing Data: %d/%u (%.2f%%) \r",(i+1), dev_info->CF_Line, (((i+1)/(float)dev_info->CF_Line)*100));

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
  printf("[%s] dev_info->CF_Line: %u\n", __func__, dev_info->CF_Line);
#endif

  for(i = 0; i < dev_info->CF_Line; i++)
  {
    printf("Verify Data: %d/%u (%.2f%%) \r",(i+1), dev_info->CF_Line, (((i+1)/(float)dev_info->CF_Line)*100));

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
  int dev_cnts=0;
  struct cpld_dev_info* dev_list;
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

  dev_cnts += get_lattice_dev_list(&dev_list);

  for (i = 0; i < dev_cnts; i++) {
    if (dr_data[0] == dev_list[i].dev_id) {
      ret = 0;
      break;
    }
  }

  return ret;
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
cpld_dev_open_jtag(void *_attr)
{
  ast_jtag_set_mode(JTAG_XFER_HW_MODE);
  return ast_jtag_open();
}

int 
cpld_dev_close_jtag()
{
  ast_jtag_close();
  return 0;
}

int
common_cpld_Get_Ver_jtag(unsigned int *ver)
{
  // support XO2, XO3, NX Family
  int ret;
  unsigned int dr_data[4]={0};

  ret = LCMXO2Family_cpld_Check_ID();
  if (ret < 0)
  {
    printf("[%s] Unknown Device ID!\n", __func__);
    goto error_exit;
  }

  //Shift in READ USERCODE(0xC0) instruction;
  ret = ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_USERCODE);
  if (ret < 0)
  {
    printf("[%s] Shift READ USERCODE(0xC0) instruction Error!\n", __func__);
    goto error_exit;
  }

#ifdef CPLD_DEBUG
  printf("[%s] READ USERCODE(0xC0)\n", __func__);
#endif

  //Read UserCode
  dr_data[0] = 0;
  ret = ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, dr_data);
  if (ret < 0)
  {
    printf("[%s] Read Usercode Error!\n", __func__);
    goto error_exit;
  }
  *ver = dr_data[0];

error_exit:
  return ret;
}

int
LCMXO2Family_cpld_update_jtag(FILE *jed_fd, char* key, char is_signed)
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

int
common_cpld_Get_id_jtag(unsigned int *dev_id)
{
  // support XO2, XO3, NX Family
  unsigned int dr_data[4] = {0};

//  ast_jtag_run_test_idle(0, JTAG_STATE_TLRESET, 3);
  //Check the IDCODE_PUB
  ast_jtag_sir_xfer(JTAG_STATE_TLRESET, LATTICE_INS_LENGTH, LCMXO2_IDCODE_PUB);
  dr_data[0] = 0x0;
  ast_jtag_tdo_xfer(JTAG_STATE_TLRESET, 32, dr_data);

  *dev_id = dr_data[0];

  return 0;
}
