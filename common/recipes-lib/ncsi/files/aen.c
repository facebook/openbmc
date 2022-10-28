/*
 *
 * Copyright 2018-present Facebook. All Rights Reserved.
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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <openbmc/kv.h>
#include "aen.h"


// helper function to check if a given buf is a valid AEN
int
is_aen_packet(AEN_Packet *buf)
{
  return (  (buf->MC_ID == 0x00)
         && (buf->Header_Revision == 0x01)
         && (buf->IID == 0x00));
}


// enable platform-specific AENs
void
enable_default_aens() {
  char cmd[64] = {0};

  memset(cmd, 0, sizeof(cmd));
  sprintf(cmd, "/usr/bin/enable-aen.sh");
  if (system(cmd) != 0) {
    syslog(LOG_ERR, "Enabling AEN failed");
  }

  return;
}


// generic function to print AEN payload to syslog
void log_aen_payload(AEN_Packet *buf, unsigned char log_level) {
  char logbuf[512];
  int nleft = sizeof(logbuf);
  int nwrite = 0;
  int i = 0;
  int j = 0;
  syslog(log_level, "log_aen_payload");
  nwrite = snprintf(logbuf, nleft, "AEN type 0x%02x, payload:", buf->AEN_Type);
  i += nwrite;
  nleft -= nwrite;

  for (j=0; j<buf->Payload_Length; ++j) {
    nwrite = snprintf(logbuf + i, nleft, "0x%04x ", ntohs(buf->Optional_AEN_Data[j]));
    i += nwrite;
    nleft -= nwrite;
  }

  syslog(log_level, "%s", logbuf);

  return;
}

// return values
//     0 - success
//     NCSI_IF_REINIT - NCSI reinited, restart required
int
process_NCSI_AEN(AEN_Packet *buf)
{
  unsigned long manu_id=0;
  //unsigned char host_err_event_num=0;
  unsigned char host_err_type=0;
  unsigned char host_err_len=0;
  unsigned char req_rst_type=0;
  char logbuf[512];
  unsigned char log_level = LOG_NOTICE;
  int i=0;
  int ret = 0;

#ifdef DEBUG
  // print AEN packet content
  i += sprintf(logbuf, "AEN Packet:");
  i += sprintf(logbuf + i, "DA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->DA[0]), ntohs(buf->DA[1]), ntohs(buf->DA[2]),
          ntohs(buf->DA[3]), ntohs(buf->DA[4]), ntohs(buf->DA[5]));
  i += sprintf(logbuf + i, "SA[6]: %x %x %x %x %x %x   ",
          ntohs(buf->SA[0]), ntohs(buf->SA[1]), ntohs(buf->SA[2]),
          ntohs(buf->SA[3]), ntohs(buf->SA[4]), ntohs(buf->SA[5]));
  i += sprintf(logbuf + i, "EtherType: 0x%04x   ",
          ntohs(buf->EtherType));
  i += sprintf(logbuf + i, "MC_ID: %d   ", ntohs(buf->MC_ID));
  i += sprintf(logbuf + i, "Header_Revision: 0x%x   ",
          ntohs(buf->Header_Revision));
  i += sprintf(logbuf + i, "IID: %d   ", ntohs(buf->IID));
  i += sprintf(logbuf + i, "Command: 0x%x   ", ntohs(buf->Command));
  i += sprintf(logbuf + i, "Channel_ID: %d   ",
          ntohs(buf->Channel_ID));
  i += sprintf(logbuf + i, "Payload_Length: %d   ",
          ntohs(buf->Payload_Length));
  i += sprintf(logbuf + i, "AEN_Type: 0x%x   ", ntohs(buf->AEN_Type));
  for (int idx=0; idx<((buf->Payload_Length-4)/2); idx++) {
    if (idx >= MAX_AEN_DATA_IN_SHORT)
      break;
    i += sprintf(logbuf + i, " data[%d]=0x%x",
            idx, ntohs(buf->Optional_AEN_Data[idx]));
  }
  syslog(LOG_NOTICE, "%s", logbuf);
#endif


  if (!is_aen_packet(buf)) {
    // Invalid AEN packet
    syslog(LOG_NOTICE, "Invalid NCSI AEN rcvd: MC_ID=0x%x Rev=0x%x IID=0x%x\n",
           buf->MC_ID, buf->Header_Revision, buf->IID);
    return 0;
  }

  buf->Payload_Length = ntohs(buf->Payload_Length);

  i = sprintf(logbuf, "NIC Event: ");

  if (buf->AEN_Type < AEN_TYPE_OEM) {
    // DMTF standard AENs
    switch (buf->AEN_Type) {
      case AEN_TYPE_LINK_STATUS_CHANGE:
        log_level = LOG_WARNING;
        i += sprintf(logbuf + i, ", LinkStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        break;

      case AEN_TYPE_CONFIGURATION_REQUIRED:
        log_level = LOG_WARNING;
        i += sprintf(logbuf + i, ", Configuration Required");
        syslog(log_level, "%s", logbuf);

        // set flag indicate all threads to exit
        return NCSI_IF_REINIT;
        break;

      case AEN_TYPE_HOST_NC_DRIVER_STATUS_CHANGE:
        i += sprintf(logbuf + i, ", DriverStatus=0x%04x-%04x",
        ntohs(buf->Optional_AEN_Data[0]),
        ntohs(buf->Optional_AEN_Data[1]));
        break;

      case AEN_TYPE_MEDIUM_CHANGE:
        i += sprintf(logbuf + i, ", Medium Change");
        break;

      case AEN_TYPE_PENDING_PLDM_REQUEST:
        i += sprintf(logbuf + i, ", Pending PLDM Request");
        break;

      default:
        i += sprintf(logbuf + i, ", Unknown AEN Type %d", buf->AEN_Type);
        log_aen_payload(buf, LOG_WARNING);
    }
  } else {
    // OEM AENs

    char nic_vendor_iana[MAX_VALUE_LEN] = {0};
    ret = kv_get("nic_vendor", nic_vendor_iana, NULL, 0);

    if (!ret) {
      // https://www.iana.org/assignments/enterprise-numbers/enterprise-numbers
      manu_id = strtol(nic_vendor_iana, NULL, 10);
    } else {
      // if kv_get fails, attempt to manually decode Vendor ID from AEN raw data
      manu_id = ntohs(buf->Optional_AEN_Data[0]) << 16 |
                ntohs(buf->Optional_AEN_Data[1]);
    }
    if (manu_id == BCM_IANA) {
      switch (buf->AEN_Type) {
        case NCSI_AEN_TYPE_OEM_BCM_HOST_ERROR:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM Host Error");
          //host_err_event_num = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          host_err_type = ntohs(buf->Optional_AEN_Data[4])>>8;
          host_err_len  = ntohs(buf->Optional_AEN_Data[4])&0xFF;
          if (host_err_type == BCM_HOST_ERR_TYPE_UNGRACEFUL_HOST_SHUTDOWN) {
            i += sprintf(logbuf + i, ", HostId=0x%x",
            ntohs(buf->Optional_AEN_Data[5]));
            i += sprintf(logbuf + i, " DownCnt=0x%04x",
            ntohs(buf->Optional_AEN_Data[6]));
          } else {
            i += sprintf(logbuf + i, ", Unknown HostErrType=0x%x Len=0x%x",
                   host_err_type, host_err_len);
          }
          break;

        case NCSI_AEN_TYPE_OEM_BCM_RESET_REQUIRED:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM NIC Reset Required");
          req_rst_type = ntohs(buf->Optional_AEN_Data[3])&0xFF;
          i += sprintf(logbuf + i, ", ResetType=0x%02x", req_rst_type);

          //syslog(LOG_CRIT, "FRU: %d BCM NIC ERR, NIC reset required, (Type %x)",
          //       FRU_NIC, ntohs(buf->Optional_AEN_Data[3]));
          break;
       case NCSI_AEN_TYPE_OEM_BCM_HOST_DECOMMISSIONED:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", BCM Host Decommissioned");
          i += sprintf(logbuf + i, ", HostId=0x%x",
               ntohs(buf->Optional_AEN_Data[3]));

          // hack: to ensure this is picked up by machinechecker,  broadcast this
          // error message to all FRUs since machinechecker runs on host and only
          // check for its own FRU
          for (int k=0; k<7; ++k)
              syslog(LOG_CRIT, "FRU: %d BCM NIC ERR, Host Decommissioned, Host %x",
                     k, ntohs(buf->Optional_AEN_Data[3]));

          break;

        default:
          log_level = LOG_WARNING;
          i += sprintf(logbuf + i, ", Unknown BCM AEN Type 0x%x", buf->AEN_Type);
          log_aen_payload(buf, log_level);
      }
    } else if (manu_id == MLX_IANA) {
      switch (buf->AEN_Type) {
        case NCSI_AEN_TYPE_OEM_MLX_THERMAL_EVENT:
          i += sprintf(logbuf + i, ", MLX thermal event detected");
          log_aen_payload(buf, log_level);
          break;
        case NCSI_AEN_TYPE_OEM_MLX_HIGH_MODULE_CURRENT:
          i += sprintf(logbuf + i, ", MLX high module current detected");
          break;
        case NCSI_AEN_TYPE_OEM_MLX_DEVICE_SELF_RESET:
          log_level = LOG_CRIT;
          i += sprintf(logbuf + i, ", MLX Device Self Reset detected");
          break;
        default:
          log_level = LOG_WARNING;
          i += sprintf(logbuf + i, ", Unknown MLX AEN Type 0x%x", buf->AEN_Type);
          log_aen_payload(buf, log_level);
      }
    } else {
      log_level = LOG_WARNING;
      i += sprintf(logbuf + i, ", Unknown OEM AEN, IANA=0x%lx, Type=0x%x", manu_id,
                   buf->AEN_Type);
      log_aen_payload(buf, log_level);
    }
  }
  syslog(log_level, "%s", logbuf);
  return 0;
}
