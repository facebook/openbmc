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
#pragma once

#include <openbmc/AmiSmbusInterfaceSrcLib.h>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace prot {
class ProtLog;
using ProtLogPtr = std::unique_ptr<ProtLog>;

class ProtLog : public PROT_LOG {
 public:
  enum class LogType {
    TIME_LOG = 0,
    DATA_LOG = 1,
  };

  enum class LogMainClass {
    SYSTEM_INFO = 0x02,
    PROTECT_DETECT_RECOVER = 0x03,
    MANIFEST_UPDATE = 0x04,
    PROT_UPDATE = 0x05,
    SPI_FILTERING = 0x06,
    BMC_COMMANDS = 0x08,
    RECOVERY_AREA_UPDATE = 0x0A,
    SMBUS_FILTERING = 0x0B,
  };

  ProtLog() = delete;
  explicit ProtLog(const PROT_LOG& log) : PROT_LOG(log) {}
  virtual ~ProtLog() {}

  std::string toHexStr(std::string_view sep = " ") const;
  virtual std::string toStr() const;

  static ProtLogPtr create(const PROT_LOG& log);
};

#define PROT_DEFINE_LOG_CLASS(cls)                      \
  class cls : public ProtLog {                          \
   public:                                              \
    explicit cls(const PROT_LOG& log) : ProtLog(log) {} \
    virtual std::string toStr() const override;         \
  }

PROT_DEFINE_LOG_CLASS(BasicLog);
PROT_DEFINE_LOG_CLASS(SystemInfoLog);
PROT_DEFINE_LOG_CLASS(ProtectDetectRecoverLog);
PROT_DEFINE_LOG_CLASS(ManifestUpdateLog);
PROT_DEFINE_LOG_CLASS(ProtUpdateLog);
PROT_DEFINE_LOG_CLASS(SpiFilteringLog);
PROT_DEFINE_LOG_CLASS(BmcCommandsLog);
PROT_DEFINE_LOG_CLASS(RecoveryAreaUpdateLog);
PROT_DEFINE_LOG_CLASS(SmbusFilteringLog);

#undef PROT_DEFINE_LOG_CLASS

} // end of namespace prot
