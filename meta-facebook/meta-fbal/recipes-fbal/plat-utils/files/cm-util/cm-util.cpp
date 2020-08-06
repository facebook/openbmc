/*
 * cm-util
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

#include <iostream>
#include <set>
#include <map>
#include <string>
#include <CLI/CLI.hpp>
#include <openbmc/pal.h>
#include <openbmc/obmc-i2c.h>

struct ModeDesc {
  uint8_t mode;
  const std::string desc;
};

const std::map<std::string, ModeDesc> mode_map{
    {"8S-0", {CM_MODE_8S_0, "8 Socket Mode, Tray 0 is primary"}},
    {"8S-1", {CM_MODE_8S_1, "8 Socket Mode, Tray 1 is primary"}},
    {"8S-2", {CM_MODE_8S_2, "8 Socket Mode, Tray 2 is primary"}},
    {"8S-3", {CM_MODE_8S_3, "8 Socket Mode, Tray 3 is primary"}},
    {"2S", {CM_MODE_2S, "2 Socket Mode"}},
    {"4S-4OU", {CM_MODE_4S_4OU, "4 Socket mode, in 4OU mode"}},
    {"4S-2OU-3", {CM_MODE_4S_2OU_3, "4 Socket mode with Tray 3 as primary"}},
    {"4S-2OU-2", {CM_MODE_4S_2OU_2, "4 Socket mode with Tray 2 as primary"}},
};

static int set_mode(uint8_t slot)
{
  int rc = 0;
  if (pal_ep_set_system_mode(slot == 4 ? MB_2S_MODE : MB_8S_MODE)) {
    std::cerr << "Set JBOG system mode failed" << std::endl;
    rc = -1;
  }

  if (cmd_cmc_set_system_mode(slot, false) < 0) {
    std::cerr << "Set CM system mode failed" << std::endl;
    rc = -1;
  }
  return rc;
}

static std::set<std::string> get_bmcs()
{
  std::set<std::string> ret{};
  uint8_t mode;
  if (cmd_cmc_get_config_mode(&mode)) {
    return ret;
  }
  ret.insert("tray0");
  ret.insert("emeraldpools");
  if (mode >= 0 && mode < 4) {
    ret.insert("tray1");
    ret.insert("tray2");
    ret.insert("tray3");
  } else if (mode >= 5 && mode <= 7) {
    ret.insert("tray1");
  }
  return ret;
}

static int get_mode()
{
  uint8_t def, cur, ch, jp;
  if (cmd_cmc_get_config_mode_ext(&def, &cur, &ch, &jp)) {
    return -1;
  }
  auto get_mode_desc = [](uint8_t md) {
    std::string desc = "Unknown mode: " + std::to_string(int(md));
    for (const auto &pair : mode_map) {
      if (md == pair.second.mode) {
        desc = pair.first + ": " + pair.second.desc;
        break;
      }
    }
    return desc;
  };

  std::cout << "Default: " << get_mode_desc(def) << std::endl;
  std::cout << "Current: " << get_mode_desc(cur) << std::endl;
  if (ch != 0xff) {
    std::cout << "Changed: " << get_mode_desc(ch) << std::endl;
  }
  const std::map<int, std::string> jumper_desc = {
    {0, "JP3"},
    {1, "JP4"},
    {2, "JP5"},
    {3, "JP6"},
    {4, "JP7"}
  };
  for (const auto& pair : jumper_desc) {
    bool st = jp & (1 << pair.first);
    std::cout << pair.second << ": " << st << std::endl;
  }
  return 0;
}

static int sled_cycle()
{
  int rc = 0;
  if (pal_ep_sled_cycle() < 0) {
    std::cerr << "Request JBOG sled-cycle failed\n";
    rc = -1;
  }
  if (cmd_cmc_sled_cycle()) {
    std::cerr << "Request chassis manager for a sled cycle failed\n";
    rc = -1;
  }
  return rc;
}

static int
get_pos() {
  uint8_t pos;
  if (cmd_cmc_get_mb_position(&pos)) {
    return -1;
  }
  std::cout << int(pos) << std::endl;
  return 0;
}

static int
reset_bmc(const std::string& bmc)
{
  if (bmc == "emeraldpools") {
    int fd = i2c_cdev_slave_open(0x6, 0x41, 0);
    if (fd < 0)
      return -1;
    int rc = i2c_smbus_write_byte_data(fd, 0x3, 0xfe);
    rc |= i2c_smbus_write_byte_data(fd, 0x3, 0xff);
    close(fd);
    return rc ? -1 : 0;
  }
  const std::map<std::string, int> bmc_map = {
    {"tray0", 0},
    {"tray1", 1},
    {"tray2", 2},
    {"tray3", 3}
  };
  if (cmd_cmc_reset_bmc((uint8_t)bmc_map.at(bmc))) {
    return -1;
  }
  return 0;
}

int
main(int argc, char **argv) {
  int rc = -1;
  bool do_cycle = false;
  CLI::App app("Chassis Management Utility");
  app.failure_message(CLI::FailureMessage::help);

  std::string op_mode;
  std::set<std::string> allowed_modes;
  std::string desc = "Set chassis operating mode\n";
  for (const auto& pair : mode_map) {
    allowed_modes.insert(pair.first);
    desc += pair.first + ": " + pair.second.desc + "\n";
  }
  auto mode = app.add_set("--set-mode", op_mode,
                          allowed_modes,
                          desc
                         )->ignore_case();
  app.add_flag("--sled-cycle", do_cycle,
               "Perform a SLED cycle after operations (If any)");
  app.require_option();
  bool get_mode_f = false;
  app.add_flag("--get-mode", get_mode_f, "Get Chassis Operating mode");

  bool get_pos_f = false;
  app.add_flag("--get-position", get_pos_f, "Get current tray position");

  std::string bmc_to_reset{};
  auto rstbmc = app.add_set("--reset-bmc", bmc_to_reset, get_bmcs(), "Reset BMC in the specified Tray");

  CLI11_PARSE(app, argc, argv);

  if (get_mode_f) {
    return get_mode();
  }

  if (get_pos_f) {
    return get_pos();
  }

  if (rstbmc) {
    return reset_bmc(bmc_to_reset);
  }

  if (*mode) {
    rc = set_mode(mode_map.at(op_mode).mode);
  }

  if (do_cycle) {
    rc = sled_cycle();
  }

  return rc;
}
