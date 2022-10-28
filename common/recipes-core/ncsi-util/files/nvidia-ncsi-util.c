/*
 *
 * Copyright 2020-present Facebook. All Rights Reserved.
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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <zlib.h>
#include <openbmc/ncsi.h>
#include <openbmc/ncsi-nvidia.h>

#include "nvidia-ncsi-util.h"

/*
 * GetDebugData transaction data
 */
typedef struct {
  NCSI_Nvidia_Get_Debug_Data_Request req;
  FILE      *fp;
  uint16_t  type;
  uint32_t  recv_len;
} Get_Debug_Data_Txn_t;

static int get_debug_data_name_to_type(const char *name)
{
  if (!strcasecmp(name, "devdump"))
    return NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_DEV_DUMP;
  else if (!strcasecmp(name, "fwdump"))
    return NCSI_NVIDIA_GET_DEBUG_DATA_TYPE_FW_DUMP;
  else
    return -1;
}

static void init_get_debug_data_txn(Get_Debug_Data_Txn_t *gdd_txn,
                                    uint16_t type)
{
  memset(gdd_txn, 0, sizeof(*gdd_txn));

  gdd_txn->type = type;

  gdd_txn->req.header.nvidia_info_type = htons(type);
}

static NCSI_NL_RSP_T *send_get_debug_data_req(NCSI_NL_MSG_T *nl_msg,
                                              Get_Debug_Data_Txn_t *gdd_txn,
                                              ncsi_util_args_t *util_args)
{
  if (ncsi_nvidia_create_req_pkt(nl_msg, NCSI_NVIDIA_GET_DEBUG_DATA_CHANNEL_ID,
                               NCSI_NVIDIA_GET_DEBUG_DATA_CMD_ID,
                               NCSI_NVIDIA_GET_DEBUG_DATA_PARA,
                               sizeof(gdd_txn->req), &gdd_txn->req)) {
    printf("Error: failed to create packet\n");
    return NULL;
  }

  /* Override the netdev if user has specified one */
  ncsi_util_set_pkt_dev(nl_msg, util_args, NULL);

  /* Send */
  return send_nl_msg(nl_msg);
}

static int handle_get_debug_data_resp(NCSI_NL_RSP_T *nl_resp,
                                      Get_Debug_Data_Txn_t *gdd_txn,
                                      bool *txn_completed)
{
  NCSI_Nvidia_Get_Debug_Data_Response *gdd_resp = ncsi_get_nvidia_resp(nl_resp);
  void *data;
  size_t data_len;

  /*
   * Check response
   */
  /* Check NC-SI response code */
  if (get_cmd_status(nl_resp) != RESP_COMMAND_COMPLETED) {
    print_ncsi_completion_codes(nl_resp);
    return -1;
  }

  /* Check NVIDIA OEM response */
  if (ncsi_nvidia_check_resp(nl_resp, NCSI_NVIDIA_GET_DEBUG_DATA_CMD_ID,
                           sizeof(*gdd_resp))) {
    ncsi_nvidia_print_resp_nvidia_hdr(nl_resp);
    return -1;
  }
  // For debug usage
  // else {
  //   ncsi_nvidia_print_resp(nl_resp);
  // }

  printf("\rNext handle: 0x%06x",ntohl(gdd_resp->next_data_handle));
  fflush(stdout);

  /*
   * Process response data
   */
  data = gdd_resp->debug_data;
  data_len = nl_resp->hdr.payload_length - sizeof(*gdd_resp);

  if (data_len > 0) {
    size_t n;

    /* Debug data received */
    gdd_txn->recv_len += data_len;

    /* Save data to file */
    n = fwrite(data, 1, data_len, gdd_txn->fp);
    if (n < data_len) {
      printf("Error: failed to write %u data bytes to file, rv %d\n",
             data_len, n);
      return -1;
    }
  }

  /*
   * Check if transaction is completed
   */
  if (gdd_resp->next_data_handle != 0) {
    gdd_txn->req.data_handle = gdd_resp->next_data_handle;
  } else {
    *txn_completed = true;
  }

  return 0;
}

static int get_debug_data(int type, const char *file_name,
                          ncsi_util_args_t *util_args)
{
  Get_Debug_Data_Txn_t gdd_txn;
  NCSI_NL_MSG_T *nl_msg = NULL;
  NCSI_NL_RSP_T *nl_resp = NULL;
  bool txn_completed = false;
  int ret = -1;

  init_get_debug_data_txn(&gdd_txn, type);

  /* Open the file */
  gdd_txn.fp = fopen(file_name, "w+");
  if (!gdd_txn.fp) {
    printf("Error: cannot create file %s\n", file_name);
    goto done;
  }

  nl_msg = calloc(1, sizeof(*nl_msg));
  if (!nl_msg) {
    printf("Error: failed to allocate nl_msg of %d bytes\n", sizeof(*nl_msg));
    goto done;
  }

  while (!txn_completed) {

    /* Send and receive */
    nl_resp = send_get_debug_data_req(nl_msg, &gdd_txn, util_args);
    if (!nl_resp)
      break;

    /* Handle response */
    if (handle_get_debug_data_resp(nl_resp, &gdd_txn, &txn_completed))
      break;

    free(nl_resp);
    nl_resp = NULL;
  }

  if (txn_completed) {
    printf("\nGetDebugData completed\n");
    ret = 0;
  }

done:
  if (gdd_txn.fp)
    fclose(gdd_txn.fp);
  if (nl_msg)
    free(nl_msg);
  if (nl_resp)
    free(nl_resp);

  return ret;
}

static void nvidia_ncsi_util_usage(void)
{
  printf("Usage: ncsi-util -m nvidia [options]\n\n");
  printf("       -h             This help\n");
  ncsi_util_common_usage();
  printf("       -d [type]      GetDebugData\n");
  printf("                        devdump  - Device Info dump\n");
  printf("                        fwdump  - FW Info dump\n");
  printf("       -o [file]      Output file\n");
  printf("\nSample commands:\n");
  printf("       ncsi-util -m nvidia -d devdump -o /tmp/nic.dev\n");
  printf("       ncsi-util -m nvidia -d fwdump -o /tmp/nic.fw\n");
  printf("\n");
}

int nvidia_ncsi_util_handler(int argc, char **argv, ncsi_util_args_t *util_args)
{
  int getdbgdata_type = -1;
  const char *ofile_name = NULL;
  int ret = -1;
  int argflag;

  while ((argflag = getopt(argc, argv, "hd:o:" NCSI_UTIL_COMMON_OPT_STR)) != -1)
  {
    switch(argflag) {
    case 'h':
            nvidia_ncsi_util_usage();
            return 0;
    case 'd':
            getdbgdata_type = get_debug_data_name_to_type(optarg);
            if (getdbgdata_type < 0) {
              printf("Error: invalid GetDebugData type %s\n", optarg);
              goto err_exit;
            }
            break;
    case 'o':
            ofile_name = optarg;
            break;
    case NCSI_UTIL_GETOPT_COMMON_OPT_CHARS:
            // Already handled
            break;
    default :
            goto err_exit;
    }
  }

  if (getdbgdata_type >= 0) {
    if (!ofile_name) {
      printf("Error: missing output file argument\n");
      goto err_exit;
    }
    return get_debug_data(getdbgdata_type, ofile_name, util_args);
  }

  /* No command was done. Fall through to show the usage */

err_exit:
  nvidia_ncsi_util_usage();
  return ret;
}
