/*
 * me-util
 *
 * Copyright 2015-present Facebook. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <facebook/bic.h>
#include <openbmc/ipmi.h>

#define  MAX_PROC_ID 31
#define  MAX_MC_IDX 19
#define  MAX_RETRY 0
#define  ME_COLD_RESET_DELAY 5

#define LOGFILE "/tmp/me-util.log"
#define CRASHDUMP_FILE "/mnt/data/crashdump_slot1"

#define  IA32_MC_CTL_base 0x0400
#define  IA32_MC_CTL2_base 0x0280
#define  IA32_MC_STATUS_base 0x0401
#define  IA32_MC_ADDR_base 0x0402
#define  IA32_MC_MISC_base 0x0403
#define  IA32_MCG_CAP 0x0179
#define  IA32_MCG_STATUS 0x017A
#define  IA32_MCG_CONTAIN 0x0178

static void
print_usage_help(void) {
  printf("Usage: me-util <server> <netfn/LUN> <cmd> <data bytes to send>\n");
  printf("Usage: me-util <server> 48 coreid\n");
  printf("Usage: me-util <server> 48 msr\n");
  printf("       *48 coreid/msr data will be saved at /mnt/data/crashdump_slot1\n");
}

int crash_dump_msr(void) {
  FILE *fp = NULL;
  int processorid = 0, retry = 0, response = 0, comp = 1,
  mc_index = 0, msr_offset = 0;
  uint16_t param, param0, param1, cmdout;
  uint8_t slot_id = 1;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t check; //for check 4th response data
  int i = 0;

  fp = fopen(CRASHDUMP_FILE, "a+");
  if (!fp) {
      printf("File open Fail\n");
      return -1;
    }
  fprintf(fp, "\n");
  fprintf(fp, "%s\n", "MSR DUMP:");
  fprintf(fp, "%s\n", "=========");
  fprintf(fp, "\n");

  while( mc_index <= MAX_MC_IDX ) {
    fprintf(fp, "********************************************************\n");
    fprintf(fp, "*                    MC index %02d                       *\n", mc_index);
    fprintf(fp, "********************************************************\n");

//////////////////////////////////////////////////////////////////////////////////////////////////////////
    fprintf(fp, "   <<< IA32_MC%d_CTL, ProcessorID from 0 to %d >>> \n", mc_index, MAX_PROC_ID);
    processorid = 0;
    retry = 0;
    while( processorid <= MAX_PROC_ID ) {
      param = IA32_MC_CTL_base + msr_offset;
      param0 = ( param & 0xFF );
      param1 = ( param >> 8 );

      tbuf[0] = 0xB8;
      tbuf[1] = 0x40;
      tbuf[2] = 0x57;
      tbuf[3] = 0x01;
      tbuf[4] = 0x00;
      tbuf[5] = 0x30;
      tbuf[6] = 0x05;
      tbuf[7] = 0x09;
      tbuf[8] = 0xb1;
      tbuf[9] = 0x00;
      tbuf[10] = processorid;
      tbuf[11] = param0;
      tbuf[12] = param1;
      tlen = 13;
      comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      check = rbuf[3];
      if( (( check == 0x80 ) || ( check = 0x81 ) || ( check == 0x81 ) || ( comp != 0 )) &&  ( retry < MAX_RETRY ) ) {
        retry++;
        sleep(1);
      }
      else {
        for ( i = 0 ; i < rlen ; i ++ )
          fprintf(fp, "%02X ", rbuf[i]);
        fprintf(fp, "\n");
        processorid++;
        retry = 0;
      }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////
    fprintf(fp, "   <<< IA32_MC%d_CTL2, ProcessorID from 0 to %d >>> \n", mc_index, MAX_PROC_ID);
    processorid = 0;
    retry = 0;
    while( processorid <= MAX_PROC_ID ) {
      param = IA32_MC_CTL2_base + mc_index;
      param0 = ( param & 0xFF );
      param1 = ( param >> 8 );

      tbuf[10] = processorid;
      tbuf[11] = param0;
      tbuf[12] = param1;

      comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      check = rbuf[3];
      if( (( check == 0x80 ) || ( check = 0x81 ) || ( check == 0x81 ) || ( comp != 0 )) &&  ( retry < MAX_RETRY ) ) {
        retry++;
        sleep(1);
      }
      else {
        for ( i = 0 ; i < rlen ; i ++ )
          fprintf(fp, "%02X ", rbuf[i]);
        fprintf(fp, "\n");
        processorid++;
        retry = 0;
      }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////
    fprintf(fp, "   <<< IA32_MC%d_STATUS, ProcessorID from 0 to %d >>> \n", mc_index, MAX_PROC_ID);
    processorid = 0;
    retry = 0;
    while( processorid <= MAX_PROC_ID ) {
      param = IA32_MC_STATUS_base + msr_offset;
      param0 = ( param & 0xFF );
      param1 = ( param >> 8 );

      tbuf[10] = processorid;
      tbuf[11] = param0;
      tbuf[12] = param1;

      comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      check = rbuf[3];
      if( (( check == 0x80 ) || ( check = 0x81 ) || ( check == 0x81 ) || ( comp != 0 )) &&  ( retry < MAX_RETRY ) ) {
        retry++;
        sleep(1);
      }
      else {
        for ( i = 0 ; i < rlen ; i ++ )
          fprintf(fp, "%02X ", rbuf[i]);
        fprintf(fp, "\n");
        processorid++;
        retry = 0;
      }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////
    fprintf(fp, "   <<< IA32_MC%d_ADDR, ProcessorID from 0 to %d >>> \n", mc_index, MAX_PROC_ID);
    processorid = 0;
    retry = 0;
    while( processorid <= MAX_PROC_ID ) {
      param = IA32_MC_ADDR_base + msr_offset;
      param0 = ( param & 0xFF );
      param1 = ( param >> 8 );

      tbuf[10] = processorid;
      tbuf[11] = param0;
      tbuf[12] = param1;

      comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      check = rbuf[3];
      if( (( check == 0x80 ) || ( check = 0x81 ) || ( check == 0x81 ) || ( comp != 0 )) &&  ( retry < MAX_RETRY ) ) {
        retry++;
        sleep(1);
      }
      else {
        for ( i = 0 ; i < rlen ; i ++ )
          fprintf(fp, "%02X ", rbuf[i]);
        fprintf(fp, "\n");
        processorid++;
        retry = 0;
      }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////
    fprintf(fp, "   <<< IA32_MC%d_MISC, ProcessorID from 0 to %d >>> \n", mc_index, MAX_PROC_ID);
    processorid = 0;
    retry = 0;
    while( processorid <= MAX_PROC_ID ) {
      param = IA32_MC_MISC_base + msr_offset;
      param0 = ( param & 0xFF );
      param1 = ( param >> 8 );

      tbuf[10] = processorid;
      tbuf[11] = param0;
      tbuf[12] = param1;

      comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
      check = rbuf[3];
      if( (( check == 0x80 ) || ( check = 0x81 ) || ( check == 0x81 ) || ( comp != 0 )) &&  ( retry < MAX_RETRY ) ) {
        retry++;
        sleep(1);
      }
      else {
        for ( i = 0 ; i < rlen ; i ++ )
          fprintf(fp, "%02X ", rbuf[i]);
        fprintf(fp, "\n");
        processorid++;
        retry = 0;
      }
    }

    mc_index++;
    msr_offset+4;
  }

  fclose(fp);
  return 0;
}

int crash_dump_coreid(void) {
  FILE *fp = NULL;
  int processorid = 0, retry = 0, response = 0, comp = 1,
  mc_index = 0, msr_offset = 0;
  uint16_t param, param0, param1, cmdout;
  uint8_t slot_id = 1;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  uint8_t check; //for check 4th response data
  int i = 0;

  fp = fopen(CRASHDUMP_FILE, "a+");
  if (!fp) {
      printf("File Open Fail\n");
      return -1;
    }
  fprintf(fp, "\n");
  fprintf(fp, "%s\n", "CPU COREID DUMP:");
  fprintf(fp, "%s\n", "================");
  fprintf(fp, "\n");

  //PECI RdPkgConfig() "CPUID Read"
  fprintf(fp, "< CPUID Read >\n");
  tbuf[0] = 0xB8;
  tbuf[1] = 0x40;
  tbuf[2] = 0x57;
  tbuf[3] = 0x01;
  tbuf[4] = 0x00;
  tbuf[5] = 0x30;
  tbuf[6] = 0x05;
  tbuf[7] = 0x05;
  tbuf[8] = 0xa1;
  tbuf[9] = 0x00;
  tbuf[10] = 0x00;
  tbuf[11] = 0x00;
  tbuf[12] = 0x00;
  tlen = 13;
  comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  for ( i = 0 ; i < rlen ; i ++ )
    fprintf(fp, "%02X ", rbuf[i]);
  fprintf(fp, "\n");

  //PECI RdPkgConfig() "CPU Microcode Update Revision Read"
  fprintf(fp, "< CPU Microcode Update Revision Read >\n");
  tbuf[11] = 0x04;
  comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  for ( i = 0 ; i < rlen ; i ++ )
    fprintf(fp, "%02X ", rbuf[i]);
  fprintf(fp, "\n");

  //PECI RdPkgConfig() "MCA ERROR SOURCE LOG Read"
  fprintf(fp, "< MCA ERROR SOURCE LOG Read -- The socket which MCA_ERR_SRC_LOG[30]=0 is the socket that asserted IERR first >\n");
  tbuf[11] = 0x05;
  comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  for ( i = 0 ; i < rlen ; i ++ )
    fprintf(fp, "%02X ", rbuf[i]);
  fprintf(fp, "\n");

  //PECI RdPkgConfig() "Core ID IERR"
  //echo "< Core ID IERR -- determine whether a core in the failing socket asserted an IERR, completion data[3]=1 if a core caused the IERR, data[2:0] is the core ID, and save the value for matching purpose >"
  //ipmitool -H $1 -U $user -P $passwd -b 6 -t 0x2c raw 0x2E 0x40 0x57 0x01 0x00 $2 0x05 0x05 0xa1 0x00 0x27 0x08 0x08

  fprintf(fp, "********************************************************\n");
  fprintf(fp, "*                   IERRLOGGINGREG                     *\n");
  fprintf(fp, "********************************************************\n");
  tbuf[0] = 0xB8;
  tbuf[1] = 0x44;
  tbuf[2] = 0x57;
  tbuf[3] = 0x01;
  tbuf[4] = 0x00;
  tbuf[5] = 0x40;
  tbuf[6] = 0xA4;
  tbuf[7] = 0x50;
  tbuf[8] = 0x18;
  tbuf[9] = 0x00;
  tbuf[10] = 0x03;
  tlen = 11;
  comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  for ( i = 0 ; i < rlen ; i ++ )
    fprintf(fp, "%02X ", rbuf[i]);
  fprintf(fp, "\n");

  fprintf(fp, "********************************************************\n");
  fprintf(fp, "*                   MCERRLOGGINGREG                    *\n");
  fprintf(fp, "********************************************************\n");
  tbuf[0] = 0xB8;
  tbuf[1] = 0x44;
  tbuf[2] = 0x57;
  tbuf[3] = 0x01;
  tbuf[4] = 0x00;
  tbuf[5] = 0x40;
  tbuf[6] = 0xA8;
  tbuf[7] = 0x50;
  tbuf[8] = 0x18;
  tbuf[9] = 0x00;
  tbuf[10] = 0x03;
  tlen = 11;
  comp = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  for ( i = 0 ; i < rlen ; i ++ )
    fprintf(fp, "%02X ", rbuf[i]);
  fprintf(fp, "\n");

  fclose(fp);
  return 0;
}

int
main(int argc, char **argv) {
  uint8_t slot_id;
  uint8_t tbuf[256] = {0x00};
  uint8_t rbuf[256] = {0x00};
  uint8_t tlen = 0;
  uint8_t rlen = 0;
  int i;
  int ret;
  int logfd;
  int len;
  char log[128];
  char temp[8];

  if (argc < 4) {
    goto err_exit;
  }

  if (!strcmp(argv[1], "server")) {
    slot_id = 1;
  } else {
    goto err_exit;
  }

  if (!strcmp(argv[2], "48")) {
    if (!strcmp(argv[3], "msr")) {
      ret = crash_dump_msr();
      return ret;
    }
    else if (!strcmp(argv[3], "coreid")) {
      ret = crash_dump_coreid();
      return ret;
    }
    else {
      goto err_exit;
    }
  }

  for (i = 2; i < argc; i++) {
    tbuf[tlen++] = (uint8_t)strtoul(argv[i], NULL, 0);
  }

#if 1
  ret = bic_me_xmit(slot_id, tbuf, tlen, rbuf, &rlen);
  if (ret) {
    return ret;
  }
#endif

  // memcpy(rbuf, tbuf, tlen);
  //rlen = tlen;

  memset(log, 0, 128);
  for (i = 0; i < rlen; i++) {
    printf("%02X ", rbuf[i]);
    memset(temp, 0, 8);
    sprintf(temp, "%02X ", rbuf[i]);
    strcat(log, temp);
  }
  printf("\n");
  sprintf(temp, "\n");
  strcat(log, temp);

  errno = 0;

  logfd = open(LOGFILE, O_CREAT | O_WRONLY);
  if (logfd < 0) {
    syslog(LOG_WARNING, "Opening a tmp file failed. errno: %d", errno);
    return -1;
  }

  len = write(logfd, log, strlen(log));
  if (len != strlen(log)) {
    syslog(LOG_WARNING, "Error writing the log to the file");
    return -1;
  }

  close(logfd);

  return 0;
err_exit:
  print_usage_help();
  return -1;
}
