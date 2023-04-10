/*
 *
 * Copyright 2022-present Facebook. All Rights Reserved.
 *
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

#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <getopt.h>
#include <openbmc/AmiSmbusInterfaceSrcLib.h>
#include <openbmc/ProtCommonInterface.hpp>
#include <openbmc/pal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include "SysCpldDevice.hpp"

enum class ProtUtilCmds {
  UNKNOW_CMD = -1,
  SENT_DATA_PACKET = 0,
  RECEIVE_DATA_PACKET = 1,
  BOOT_STATUS = 2,
  FW_UPDATE = 3,
  SENT_DATA_PACKET_NOACK = 4,
  READ_MINIFEST = 5,
  SHOW_LOG = 6,
  READ_UNIQUE_SERIAL_NUMBER = 7,
  FLASH_SYNC = 8,
  DO_RECOVERY = 9,
  DECOMMISSION = 10,
  RECOMMISSION = 11,
  SMBUSFILTER = 12,
  TIMEBOUNDDEBUG = 13,
  ATTESTATION = 14,
  READ_LEVEL0_PUBKEY = 15,
  TEST = 0xFF,
};

static int opt_json = false;

using prot::ProtDevice;
using sysCpld::CpldDevice;

static int do_send_data_packet(uint8_t fru_id, std::vector<uint8_t> data_raw) {
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;

  if (pal_get_prot_address(fru_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    std::cerr << "pal_get_prot_address failed, fru: " << fru_id << std::endl;
    return -1;
  }

  if ((data_raw.size() - 1) > DATA_LAYER_MAX_PAYLOAD) {
    std::cerr << "the max payload is " << DATA_LAYER_MAX_PAYLOAD << std::endl;
    return -1;
  }

  ProtDevice prot_dev(fru_id, dev_i2c_bus, dev_i2c_addr);

  if (!prot_dev.isDevOpen()) {
    std::cerr << "Fail to open i2c" << std::endl;
    return -1;
  }

  auto rc = prot_dev.protSendDataPacket(data_raw);
  if (rc != ProtDevice::DevStatus::SUCCESS) {
    std::cerr << "protSendDataPacket failed: " << (int)rc << std::endl;
    return -1;
  }

  return 0;
}

static int do_receive_data_packet(uint8_t fru_id) {
  DATA_LAYER_PACKET_ACK data_layer_ack{};
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;

  if (pal_get_prot_address(fru_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    std::cerr << "pal_get_prot_address failed, fru: " << fru_id << std::endl;
    return -1;
  }

  ProtDevice prot_dev(fru_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    std::cerr << "Fail to open i2c" << std::endl;
    return -1;
  }

  auto rc = prot_dev.protRecvDataPacket(data_layer_ack);
  if (rc != ProtDevice::DevStatus::SUCCESS) {
    std::cerr << "protRecvDataPacket failed:" << (int)rc << std::endl;
    return -1;
  }

  return 0;
}

static int do_get_boot_status(uint8_t fru_id) {
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;

  if (pal_get_prot_address(fru_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    std::cerr << "pal_get_prot_address failed, fru: " << fru_id << std::endl;
    return -1;
  }

  ProtDevice prot_dev(fru_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    std::cerr << "Fail to open i2c" << std::endl;
    return -1;
  }

  CpldDevice cpld_dev(fru_id, dev_i2c_bus, sysCpld::sys_cpld_addr);

  nlohmann::json j{};
  std::string auth("Auth" + std::to_string(int(fru_id)));

  bool is_module_prsnt = pal_is_prot_card_prsnt(fru_id);
  do {
    prot::BOOT_STATUS_ACK_PAYLOAD boot_sts{};

    j[auth]["Module-Present"] = is_module_prsnt;
    if (!is_module_prsnt) {
      break;
    }

    auto is_module_bypass = pal_is_prot_bypass(fru_id);
    j[auth]["Module-Mode"] = std::string(is_module_bypass ? "BYPASS" : "PFR");

    try {
      auto is_auth_cmplt = cpld_dev.isAuthComplete();
      j[auth]["Auth-Complete"] = is_auth_cmplt;
    } catch(const std::runtime_error& err) {
      j[auth]["Auth-Complete"] = nullptr;
    }

    try {
      auto is_boot_fail = cpld_dev.isBootFail();
      j[auth]["Boot-Fail"] = is_boot_fail;
    } catch(const std::runtime_error& err) {
      j[auth]["Boot-Fail"] = nullptr;
    }


    if (is_module_bypass) {
      break;
    }

    memset(&boot_sts, 0xff, sizeof(boot_sts));
    auto rc = prot_dev.portGetBootStatus(boot_sts);
    if (rc != ProtDevice::DevStatus::SUCCESS) {
      std::cerr << "portGetBootStatus failed: " << (int)rc << std::endl;
      break;
    }

    j["SPI_A"]["Status"] = fmt::format(
        "{:#04x} ({})",
        boot_sts.SPI_A,
        prot::ProtSpiInfo::spiStatusString(boot_sts.SPI_A));
    j["SPI_A"]["BIOS-Version"] =
        prot::ProtVersion::getVerString(boot_sts.SPIABiosVersionNumber);

    j["SPI_B"]["Status"] = fmt::format(
        "{:#04x} ({})",
        boot_sts.SPI_B,
        prot::ProtSpiInfo::spiStatusString(boot_sts.SPI_B));
    j["SPI_B"]["BIOS-Version"] =
       prot::ProtVersion::getVerString(boot_sts.SPIBBiosVersionNumber);

    j["Verification-Status"]["active"] = fmt::format(
        "{:#04x} ({})",
        boot_sts.ActiveVerificationStatus,
        prot::ProtSpiInfo::spiVerifyString(boot_sts.ActiveVerificationStatus));
    j["Verification-Status"]["recovery"] = fmt::format(
        "{:#04x} ({})",
        boot_sts.RecoveryVerificationStatus,
        prot::ProtSpiInfo::spiVerifyString(
            boot_sts.RecoveryVerificationStatus));

    /*XFR Z.5.0005:: Get XFR FW Versions*/
    prot::XFR_VERSION_READ_ACK_PAYLOAD prot_ver{};
    rc = prot_dev.protReadXfrVersion(prot_ver);
    if (rc != ProtDevice::DevStatus::SUCCESS) {
      std::cerr << "protReadXfrVersion failed: " << (int)rc << std::endl;
      break;
    }
    j["XFR-VERSION"]["SiliconRBP"] =
        fmt::format("{:#06x}", prot_ver.SiliconRBP);

    {
      nlohmann::json ver_tmp{};

      ver_tmp["signature"] =
          prot::ProtVersion::getVerString(prot_ver.Active.signature);
      ver_tmp["XFRVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.XFRVersion);
      ver_tmp["BuildDate"] =
          prot::ProtVersion::getDateString(prot_ver.Active.BuildDate);
      ver_tmp["BuildTime"] =
          prot::ProtVersion::getTimeString(prot_ver.Active.Time);
      ver_tmp["SFBVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.SFBVersion);
      ver_tmp["CFGVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.CFGVersion);
      ver_tmp["WorkSpaceVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.WorkSpaceVersion);
      ver_tmp["OEMVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.OEMVersion);
      ver_tmp["ODMVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Active.ODMVersion);
      j["XFR-VERSION"]["Active"] = std::move(ver_tmp);
    }

    {
      nlohmann::json ver_tmp{};
      ver_tmp["signature"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.signature);
      ver_tmp["XFRVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.XFRVersion);
      ver_tmp["BuildDate"] =
          prot::ProtVersion::getDateString(prot_ver.Recovery.BuildDate);
      ver_tmp["BuildTime"] =
          prot::ProtVersion::getTimeString(prot_ver.Recovery.Time);
      ver_tmp["SFBVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.SFBVersion);
      ver_tmp["CFGVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.CFGVersion);
      ver_tmp["WorkSpaceVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.WorkSpaceVersion);
      ver_tmp["OEMVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.OEMVersion);
      ver_tmp["ODMVersion"] =
          prot::ProtVersion::getVerString(prot_ver.Recovery.ODMVersion);
      j["XFR-VERSION"]["Recovery"] = std::move(ver_tmp);
    }
  } while (0);

  if (opt_json) {
    std::cout << j.dump(4) << std::endl;
  } else {
    std::cout << auth << " Module Present: " << std::boolalpha
              << is_module_prsnt << std::noboolalpha << std::endl;
    if (is_module_prsnt) {
      std::cout << auth
                << " Module Mode: " << j[auth]["Module-Mode"].get<std::string>()
                << std::endl;

      std::cout << auth << " Auth-Complete: " << j[auth]["Auth-Complete"] << std::endl;
      std::cout << auth << " Boot-Fail: " << j[auth]["Boot-Fail"] << std::endl;
    }

    if (j.contains("SPI_A")) {
      std::cout << "SPI_A Status: " << j["SPI_A"]["Status"].get<std::string>()
                << std::endl;
      std::cout << "SPI_A BIOS-Version: "
                << j["SPI_A"]["BIOS-Version"].get<std::string>() << std::endl;
    }

    if (j.contains("SPI_B")) {
      std::cout << "SPI_B Status: " << j["SPI_B"]["Status"].get<std::string>()
                << std::endl;
      std::cout << "SPI_B BIOS-Version: "
                << j["SPI_B"]["BIOS-Version"].get<std::string>() << std::endl;
    }

    if (j.contains("Verification-Status")) {
      std::cout << "Verification-Status active: "
                << j["Verification-Status"]["active"].get<std::string>()
                << std::endl;
      std::cout << "Verification-Status recovery: "
                << j["Verification-Status"]["recovery"].get<std::string>()
                << std::endl;
    }
    if (j.contains("XFR-VERSION")) {
      std::vector<std::string> ordered_keys{
          "signature",
          "XFRVersion",
          "BuildDate",
          "BuildTime",
          "SFBVersion",
          "CFGVersion",
          "WorkSpaceVersion",
          "OEMVersion",
          "ODMVersion"};

      for (auto& type : {"Active", "Recovery"}) {
        auto& jv = j["XFR-VERSION"][type];
        for (auto& k : ordered_keys) {
          auto prefix = fmt::format("XFR-VERSION {} {}: ", type, k);
          std::cout << prefix << jv[k].get<std::string>() << std::endl;
        }
      }
      std::cout << "XFR-VERSION SiliconRBP: "
                << j["XFR-VERSION"]["SiliconRBP"].get<std::string>()
                << std::endl;
    }
  }

  return 0;
}

static int do_show_log(uint8_t fru_id) {
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;

  if (pal_get_prot_address(fru_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    std::cerr << "pal_get_prot_address failed, fru: " << fru_id << std::endl;
    return -1;
  }

  ProtDevice prot_dev(fru_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    std::cerr << "Fail to open i2c" << std::endl;
    return -1;
  }

  size_t log_count = 0;
  {
    auto rc = prot_dev.protUfmLogReadoutEntry(log_count);
    if (rc != ProtDevice::DevStatus::SUCCESS) {
      std::cerr << "portGetBootStatus failed: " << (int)rc << std::endl;
      return -1;
    }
  }

  std::cout
      << "\n-------------------------- Result --------------------------\n";
  std::cout << fmt::format("PROT Log Count - {:d} \n", log_count);
  for (size_t index = 0; index < log_count; index++) {
    PROT_LOG prot_log;

    auto rc = prot_dev.protGetLogData(index, prot_log);
    if (rc != ProtDevice::DevStatus::SUCCESS) {
      std::cerr << fmt::format(
          "protGetLogData failed at index {:d}: {:d}\n", index, (int)rc);
      return -1;
    }

    auto log_sptr = prot::ProtLog::create(prot_log);
    std::cout << fmt::format("LOG {:d} : {}", index, log_sptr->toHexStr())
              << std::endl;
    std::cout << log_sptr->toStr() << std::endl;

    msleep(200);
  }

  return 0;
}

static int do_read_level0_pubkey(uint8_t fru_id) {
  uint8_t dev_i2c_bus = 0;
  uint8_t dev_i2c_addr = 0;

  if (pal_get_prot_address(fru_id, &dev_i2c_bus, &dev_i2c_addr) != 0) {
    std::cerr << "pal_get_prot_address failed, fru: " << fru_id << std::endl;
    return -1;
  }

  ProtDevice prot_dev(fru_id, dev_i2c_bus, dev_i2c_addr);
  if (!prot_dev.isDevOpen()) {
    std::cerr << "Fail to open i2c" << std::endl;
    return -1;
  }

  std::vector<uint8_t> pubkey;
  auto rc = prot_dev.protLevel0PubKeyRead(pubkey);
  if (rc != ProtDevice::DevStatus::SUCCESS) {
    std::cerr << "protLevel0PubKeyRead failed: " << (int)rc << std::endl;
    return -1;
  }

  std::cout << "0x";
  for(auto& v : pubkey) {
    std::cout << fmt::format("{:02x}",v);
  }
  std::cout << std::endl;

  return 0;
}

static int parse_args(
    int argc,
    char** argv,
    uint8_t* fru_id,
    ProtUtilCmds* action,
    std::vector<uint8_t>& data_raw) {
  std::set<std::string> fru_list;
  std::string fru_list_str(pal_server_list);
  std::regex pattern(R"(\s*,\s*)");
  std::string fru;

  CLI::App app("PROT-Util");
  app.failure_message(CLI::FailureMessage::help);
  app.set_help_flag();
  app.set_help_all_flag("-h,--help");

  std::copy(
      std::sregex_token_iterator(
          fru_list_str.begin(), fru_list_str.end(), pattern, -1),
      std::sregex_token_iterator(),
      std::inserter(fru_list, fru_list.begin()));
  std::set<std::string> allowed_fru(fru_list);
  app.add_option("fru", fru)->check(CLI::IsMember(allowed_fru))->required();

  auto v_flag = app.add_flag("--verbose");

  auto send_cmd = app.add_subcommand("sendDataPacket", "Send Data Packet");
  send_cmd->add_option("raw", data_raw, "Raw data to send");

  auto recv_cmd =
      app.add_subcommand("receiveDataPacket", "Receive Data Packet");

  auto status_cmd = app.add_subcommand("status", "Status");
  auto j_flag = status_cmd->add_flag("-j,--json");

  auto log_cmd = app.add_subcommand("log", "Log");

  auto read_pubkey_cmd = app.add_subcommand("read_pubkey", "Read Level0 Public Key");

  CLI11_PARSE(app, argc, argv);

  ProtDevice::setVerbose((*v_flag) ? true : false);

  if (pal_get_fru_id(const_cast<char*>(fru.c_str()), fru_id) != 0) {
    std::cerr << "fru is invalid!\n";
    std::cerr << app.help() << std::endl;
    return -1;
  }

  if (send_cmd->parsed()) {
    *action = ProtUtilCmds::SENT_DATA_PACKET;
    ProtDevice::setVerbose(true);
  } else if (recv_cmd->parsed()) {
    *action = ProtUtilCmds::RECEIVE_DATA_PACKET;
    ProtDevice::setVerbose(true);
  } else if (status_cmd->parsed()) {
    opt_json = (*j_flag) ? true : false;
    *action = ProtUtilCmds::BOOT_STATUS;
  } else if (log_cmd->parsed()) {
    *action = ProtUtilCmds::SHOW_LOG;
  } else if (read_pubkey_cmd->parsed()) {
    *action = ProtUtilCmds::READ_LEVEL0_PUBKEY;
  } else {
    std::cout << app.help() << std::endl;
    return -1;
  }

  return 0;
}

int main(int argc, char** argv) {
  uint8_t fru_id = 0;
  ProtUtilCmds cmd = ProtUtilCmds::UNKNOW_CMD;
  std::vector<uint8_t> data_raw;
  if (parse_args(argc, argv, &fru_id, &cmd, data_raw)) {
    return -1;
  }

  switch (cmd) {
    case ProtUtilCmds::SENT_DATA_PACKET:
      return do_send_data_packet(fru_id, data_raw);
    case ProtUtilCmds::RECEIVE_DATA_PACKET:
      return do_receive_data_packet(fru_id);
    case ProtUtilCmds::BOOT_STATUS:
      return do_get_boot_status(fru_id);
    case ProtUtilCmds::SHOW_LOG:
      return do_show_log(fru_id);
    case ProtUtilCmds::READ_LEVEL0_PUBKEY:
      return do_read_level0_pubkey(fru_id);
    default:
      return -1;
  }

  return 0;
}
