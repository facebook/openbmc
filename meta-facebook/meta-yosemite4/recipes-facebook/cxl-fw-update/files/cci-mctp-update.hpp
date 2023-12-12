#pragma once

/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <linux/mctp.h>

#include <cstdint>

#define MCTP_MEG_TYPE_CCI 0x8
#define DEFAULT_NET 1

enum class CCI_RETUEN_CODE
{
    CCI_SUCCESS = 0x0000,
    CCI_INVALID_INPUT = 0x0002,
    CCI_UNSUPPORTED = 0x0003,
    CCI_INTERNAL_ERROR = 0x0004,
    CCI_RETRY_REQUIRED = 0x0005,
    CCI_PAYLOAD_INVALID_LEN = 0x0016,
};

enum class ACTIVE_FW_SLOT
{
    SLOT1_FW_ACTIVE = 0x01,
    SLOT2_FW_ACTIVE,
    SLOT3_FW_ACTIVE,
    SLOT4_FW_ACTIVE,
};

enum class TRANSFER_FW_OPTION
{
    FULL_FW_TRANSFER,
    INITIATE_FW_TRANSFER,
    CONTINUE_FW_TRANSFER,
    END_TRANSFER,
    ABORT_TRANSFER,
};

enum class ACTIVATE_FW_OPTION
{
    ONLINE_ACTIVE_FW,
    NEXT_COLD_RESET_ACTIVE_FW,
};

/*CCI Command Code */
static constexpr uint16_t CCI_GET_HEALTH_INFO = 0x4200;
static constexpr uint16_t CCI_GET_FW_INFO = 0x0200;
static constexpr uint16_t CCI_TRANSFER_FW = 0x0201;
static constexpr uint16_t CCI_ACTIVATE_FW = 0x0202;

/*CCI Request paypload length */
static constexpr size_t HEALTH_INFO_REQ_PL_LEN = 0;
static constexpr size_t GET_FW_INFO_REQ_PL_LEN = 0;
static constexpr size_t TRANSFER_FW_REQ_PL_LEN = 256;
static constexpr size_t ACTIVATE_FW_REQ_PL_LEN = 2;

/*CCI Response paypload length */
static constexpr size_t HEALTH_INFO_RESP_PL_LEN = 18;
static constexpr size_t GET_FW_INFO_RESP_PL_LEN = 80;
static constexpr size_t TRANSFER_FW_RESP_PL_LEN = 0;
static constexpr size_t ACTIVATE_FW_RESP_PL_LEN = 0;

static constexpr size_t GET_FW_INFO_RESV_LEN = 13;
static constexpr size_t GET_FW_INFO_REVISION_LEN = 16;
static constexpr size_t TRANSFER_FW_RESV_LEN = 120;
static constexpr size_t TRANSFER_FW_DATA_LEN = 128;

struct mctp_cci_hdr
{
    uint8_t cci_msg_req_resp; /* 0h = Request, 1h = Response*/
    uint8_t msg_tag;
    uint8_t cci_rsv;
    uint16_t op;
    int pl_len:21;
    uint8_t rsv:2;
    uint8_t BO:1;
    uint16_t ret;
    uint16_t stat;
} __attribute__((packed));

struct mctp_cci_msg
{
    mctp_cci_hdr hdr;
    uint8_t* pl_data;
};

struct cci_fw_info_resp
{
    mctp_cci_hdr hdr;
    uint8_t fw_slot_supported;
    union
    {
        uint8_t value;
        struct
        {
            uint8_t ACTIVE_FW_SLOT:3;
            uint8_t STAGED_FW_SLOT:3;
            uint8_t RESV:2;
        } fields;
    } fw_slot_info;
    uint8_t fw_active_capability;
    uint8_t reserved[GET_FW_INFO_RESV_LEN];
    char slot1_fw_revision[GET_FW_INFO_REVISION_LEN];
    char slot2_fw_revision[GET_FW_INFO_REVISION_LEN];
    char slot3_fw_revision[GET_FW_INFO_REVISION_LEN];
    char slot4_fw_revision[GET_FW_INFO_REVISION_LEN];
} __attribute__((packed));

struct cci_transfer_fw_req
{
    uint8_t action;
    uint8_t slot;
    uint16_t reserved_1;
    uint32_t offset;
    uint8_t reserved_2[TRANSFER_FW_RESV_LEN];
    uint8_t data[TRANSFER_FW_DATA_LEN];
};

struct cci_activate_fw_req
{
    uint8_t action;
    uint8_t slot;
};

/** @brief Get CXL firmware version using CCI over MCTP and print the version
 *infomation.
 *	@param eid EID of CXL to get firmware version
 *  @return Success or Fail
 */
int get_fw_ver_cci_over_mctp(uint8_t eid);
