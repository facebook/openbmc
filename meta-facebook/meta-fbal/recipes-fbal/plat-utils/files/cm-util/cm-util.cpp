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

int
main(int argc, char **argv) {
  int rc = -1;
  bool do_cycle = false;
  CLI::App app("Chassis Management Utility");
  app.failure_message(CLI::FailureMessage::help);

  const std::map<std::string, uint8_t> mode_map{
    {"8S-0", 3},
    {"8S-1", 2},
    {"8S-2", 1},
    {"8S-3", 0},
    {"2S", 4}
  };
  std::string op_mode;
  auto mode = app.add_set("--set-mode", op_mode,
                          {"8S-0", "8S-1", "8S-2", "8S-3", "2S"},
                          "Chassis operating mode.\n"
                          "2S: 4x2S Mode\n"
                          "8S-N: 8S mode with tray N as the master"
                         )->ignore_case();
  app.add_flag("--sled-cycle", do_cycle,
               "Perform a SLED cycle after operations (If any)");
  app.require_option();

  CLI11_PARSE(app, argc, argv);

  if (*mode) {
    rc = set_mode(mode_map.at(op_mode));
  }

  if (do_cycle) {
    rc = sled_cycle();
  }

  return rc;
}
