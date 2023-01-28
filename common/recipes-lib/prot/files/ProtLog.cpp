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

#include "ProtLog.hpp"

#include <fmt/format.h>
#include <openbmc/AmiSmbusInterfaceSrcLib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace prot {

std::string ProtLog::toHexStr(std::string_view sep) const {
  auto raw_log_ptr =
      reinterpret_cast<const uint8_t*>(static_cast<const PROT_LOG*>(this));
  return fmt::format(
      "{:#04x}", fmt::join(raw_log_ptr, (raw_log_ptr + sizeof(PROT_LOG)), sep));
}

std::string ProtLog::toStr() const {
  return fmt::format("    {:02d}:{:02d}:{:02d}  ", hour, minute, second) +
      fmt::format("type:{:#04x} ", type) +
      fmt::format("class:{:#04x} ", mainclass) +
      fmt::format("subclass:{:#04x} ", subclass) +
      fmt::format("reserved:{:#04x} ", reserved) +
      fmt::format("code:{:#06x} ", code) +
      fmt::format("parameter:{:#010x} ", parameter) + '\n';
}

ProtLogPtr ProtLog::create(const PROT_LOG& log) {
  if (static_cast<LogType>(log.type) != LogType::DATA_LOG) {
    return std::make_unique<BasicLog>(log);
  }

  switch (static_cast<LogMainClass>(log.mainclass)) {
    case LogMainClass::SYSTEM_INFO:
      return std::make_unique<SystemInfoLog>(log);
      break;

    case LogMainClass::PROTECT_DETECT_RECOVER:
      return std::make_unique<ProtectDetectRecoverLog>(log);
      break;

    case LogMainClass::MANIFEST_UPDATE:
      return std::make_unique<ManifestUpdateLog>(log);
      break;

    case LogMainClass::PROT_UPDATE:
      return std::make_unique<ProtUpdateLog>(log);
      break;

    case LogMainClass::SPI_FILTERING:
      return std::make_unique<SpiFilteringLog>(log);
      break;

    case LogMainClass::BMC_COMMANDS:
      return std::make_unique<BmcCommandsLog>(log);
      break;

    case LogMainClass::RECOVERY_AREA_UPDATE:
      return std::make_unique<RecoveryAreaUpdateLog>(log);
      break;

    case LogMainClass::SMBUS_FILTERING:
      return std::make_unique<SmbusFilteringLog>(log);
      break;

    default:
      return std::make_unique<BasicLog>(log);
      break;
  }
}

std::string BasicLog::toStr() const {
  return ProtLog::toStr() + "    Log: " +
      [this]() {
        return static_cast<LogType>(type) == LogType::TIME_LOG
            ? "Time Log"
            : "Undefined Log";
      }() +
      '\n';
}

std::string SystemInfoLog::toStr() const {
  return ProtLog::toStr() + "    Log: System Info, " + [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Active {:#04x}, Backup SVN {:#04x}",
            (code & 0x00ff),
            (code & 0xff00) >> 8);

      case 2:
        return fmt::format("Current State, {:#06x}", code);

      case 3:
        return fmt::format("Current State, {:#06x}", code);

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string ProtectDetectRecoverLog::toStr() const {
  return ProtLog::toStr() + "    Log: Protect Detect Recover, " +
             [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Last Verification Status, {}", (code ? "Failure" : "Success"));

      case 2:
        return fmt::format(
            "Last Recovery Status, {}", (code ? "Failure" : "Success"));

      case 3:
        return fmt::format(
            "Is Recovery is Triggered by PlatFire, {}", (code ? "Yes" : "No"));

      case 4:
        return fmt::format(
            "Is Recovery Triggered from Bmc, {}", (code ? "Yes" : "No"));

      case 5:
        return fmt::format(
            "Is Recovery is Triggered by WDT, {}", (code ? "Yes" : "No"));

      case 6:
        return fmt::format(
            "Is Recovery is Triggered by TrustedFv, {}", (code ? "Yes" : "No"));

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string ManifestUpdateLog::toStr() const {
  return ProtLog::toStr() + "    Log: Manifest Update, " +
             [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format("Current Manifest Type, {:#06x}", code);

      case 2:
        return fmt::format(
            "Last Manifest Update Status, {}", (code ? "Failure" : "Success"));

      case 3:
        return fmt::format(
            "Is manifest updated to Backup UFM, {}", (code ? "Yes" : "No"));

      case 4:
        return fmt::format("Last Cancelled Key, {:#06x}", code);

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string ProtUpdateLog::toStr() const {
  return ProtLog::toStr() + "    Log: PROT Update, " + [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Last PROT Update Status, {}", (code ? "Failure" : "Success"));

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string SpiFilteringLog::toStr() const {
  return ProtLog::toStr() + "    Log: SPI Filtering, " +
             [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Is SPI Filtering Enabled, {}", (code ? "Yes" : "No"));

      case 2:
        return fmt::format(
            "Is SPI Filtering Disabled, {}", (code ? "Yes" : "No"));

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string BmcCommandsLog::toStr() const {
  static const std::vector log_str_vec = {
      "subclass undefined",
      "Is RECOVERY_SPI_UNLOCK_REQUEST (0x80) Received",
      "Is UPDATE_COMPLETE (0x81) Received",
      "Is UPDATE_STATUS (0x82) Received",
      "Is BOOT_STATUS (0x83) Received",
      "Is PROT_GRANT_STATUS (0x84) Received",
      "Is FW_UPDATE_INTENT_REQUEST (0x85) Received",
      "Is ON_DEMAND_CPLD_UPDATE (0x86) Received",
      "Is ENABLE_TIME_BOUND_DEBUG (0x87) Received",
      "Is DYNAMIC_REGION_SPI_UNLOCK_REQUEST (0x88) Received",
      "Is SYNC_COMPLETE_AND_BOOT_PCH (0x89) Received",
      "Is DECOMMISSION_REQUEST (0x8A) Received",
      "Is RECOMMISSION (0x8B) Received",
      "Is UFM_LOG_READOUT_ENTRY (0x8C) Received",
      "Is READ_MANIFEST_DATA (0x8E) Received",
      "Is FW_ATTESTATION_INFORMATION (0x8F) Received",
      "Is PROT_ATTESTATION_INFORMATION (0x90) Received",
      "Is READ_UNIQUE_SERIAL_NUMBER (0x91) Received",
      "Is TPM_EVENT_ATTESTATION_INFO (0x92) Received",
      "Is EXTERNAL_EXE_BLOCK (0x7) Received",
      "Is RETURN_EXTERNAL_EXE_BLOCK (0x8) Received",
      "Is PAUSE_EXE_BLOCK (0x9) Received",
      "Is NEXT_EXE_BLOCK_AUTH_PASS (0xA) Received",
      "Is EXIT_PLATFORM_AUTHORITY (0xB) Received",
      "Is START_EXE_BLOCK (0xC) Received",
      "Is RESUME_EXE_BLOCK (0xD) Received",
      "Is ENTER_MANGEMENT_MODE (0xE) Received",
      "Is LEAVE_MANAGEMENT_MODE (0xF) Received",
      "Is BMC_RESET_OPERATION (0x10) Received",
      "Is BMC_BOOT_DONE (0x17) Received",
  };

  return ProtLog::toStr() + "    Log: BMC Commands, " +
      log_str_vec[subclass < log_str_vec.size() ? subclass : 0] + '\n';
}

std::string RecoveryAreaUpdateLog::toStr() const {
  return ProtLog::toStr() + "    Log: Recovery Area Update, " +
             [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Last Recovery Update Status, {}", (code ? "Failure" : "Success"));

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

std::string SmbusFilteringLog::toStr() const {
  return ProtLog::toStr() + "    Log: Smbus Filter, " +
             [this]() -> std::string {
    switch (subclass) {
      case 1:
        return fmt::format(
            "Is SMBUS Filtering Enabled, {}", (code ? "Yes" : "No"));

      case 2:
        return fmt::format(
            "Is SMBUS Filtering Disabled, {}", (code ? "Yes" : "No"));

      default:
        return "subclass undefine";
    }
  }() + '\n';
}

} // end of namespace prot
