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
#include "cci-mctp-update.hpp"

#include <ctype.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>

int get_fw_ver_cci_over_mctp(uint8_t eid)
{
    struct sockaddr_mctp_ext addr = {};
    mctp_cci_msg txbuf = {};
    cci_fw_info_resp fw_info;

    auto sd = socket(AF_MCTP, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        std::cerr << "Failed to create socket for MCTP" << std::endl;
        return -1;
    }

    std::cerr << "Get Firmware Info for EID: " << +eid << std::endl;

    auto addrlen = sizeof(struct sockaddr_mctp);
    addr.smctp_base.smctp_family = AF_MCTP;
    addr.smctp_base.smctp_network = DEFAULT_NET;
    addr.smctp_base.smctp_addr.s_addr = eid;
    addr.smctp_base.smctp_type = MCTP_MEG_TYPE_CCI;
    addr.smctp_base.smctp_tag = MCTP_TAG_OWNER;

    txbuf.hdr.cci_msg_req_resp = 0;
    txbuf.hdr.msg_tag = 0x2;
    txbuf.hdr.op = CCI_GET_FW_INFO;
    txbuf.hdr.pl_len = 0;
    auto len = sizeof(txbuf.hdr) / sizeof(uint8_t);

    auto ret = sendto(sd, &txbuf, len, 0, (struct sockaddr*)&addr, addrlen);
    if (ret < 0)
    {
        std::cerr << "Failed to do sendto" << std::endl;
        return -1;
    }

    ret = recvfrom(sd, NULL, 0, MSG_PEEK | MSG_TRUNC, NULL, 0);
    if (ret < 0)
    {
        std::cerr << "Failed to do recvfrom" << std::endl;
        return -1;
    }
    len = ret;

    std::vector<uint8_t> rxbuf(len);

    /* receive response */
    addrlen = sizeof(addr);
    ret = recvfrom(sd, rxbuf.data(), len, MSG_TRUNC, (struct sockaddr*)&addr,
                   &addrlen);
    if (ret < 0)
    {
        std::cerr << "Failed to receive response" << std::endl;
        return -1;
    }

    if (len != sizeof(cci_fw_info_resp) / sizeof(uint8_t))
    {
        std::cerr << "Response length: " << len
                  << " for Get FW Info is not correct! " << std::endl;
        return -1;
    }
    else
    {
        memcpy(&fw_info, rxbuf.data(), len * sizeof(uint8_t));

        std::cerr << "FW Slots Supported: " << +fw_info.fw_slot_supported
                  << std::endl;
        std::cerr << "Active FW Slot: "
                  << +fw_info.fw_slot_info.fields.ACTIVE_FW_SLOT << std::endl;
        std::cerr << "Staged FW Slot: "
                  << +fw_info.fw_slot_info.fields.STAGED_FW_SLOT << std::endl;
        std::cerr << "FW Activation Capabilities: "
                  << +fw_info.fw_active_capability << std::endl;
        std::cerr << "Slot 1 FW Revision: ";
        std::cerr.write(fw_info.slot1_fw_revision, GET_FW_INFO_REVISION_LEN);
        std::cerr << std::endl;
        std::cerr << "Slot 2 FW Revision: ";
        std::cerr.write(fw_info.slot2_fw_revision, GET_FW_INFO_REVISION_LEN);
        std::cerr << std::endl;
        std::cerr << "Slot 3 FW Revision: ";
        std::cerr.write(fw_info.slot3_fw_revision, GET_FW_INFO_REVISION_LEN);
        std::cerr << std::endl;
        std::cerr << "Slot 4 FW Revision: ";
        std::cerr.write(fw_info.slot4_fw_revision, GET_FW_INFO_REVISION_LEN);
        std::cerr << std::endl;
    }

    return 0;
}
