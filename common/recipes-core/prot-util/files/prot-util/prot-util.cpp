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
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <openbmc/pal.h>

enum {
  UNKNOW_CMD = 0,
  BOOT_STATUS = 1,
};

static int opt_json = false;

static int do_get_status(uint8_t fru_id) {
  bool is_module_prsnt;
  bool is_module_bypass;
  std::string auth("Auth" + std::to_string(int(fru_id)));
  std::string sts_str;
  nlohmann::json j;

  do {
    is_module_prsnt = pal_is_prot_card_prsnt(fru_id);
    j[auth]["Module-Present"] = is_module_prsnt;
    if (!is_module_prsnt) {
      break;
    }

    is_module_bypass = pal_is_prot_bypass(fru_id);
    sts_str.assign(is_module_bypass ? "BYPASS" : "PFR");
    j[auth]["Module-Mode"] = sts_str;
    if (is_module_bypass) {
      break;
    }
  } while (0);

  if (opt_json) {
    std::cout << j.dump(4) << std::endl;
  } else {
    std::cout << auth << " Module Present: " << std::boolalpha << is_module_prsnt << std::noboolalpha << std::endl;
    if (is_module_prsnt) {
      std::cout << auth << " Module Mode: " << j[auth]["Module-Mode"].get<std::string>() << std::endl;
    }
  }

  return 0;
}

static int parse_args(
    int argc,
    char** argv,
    uint8_t* fru_id,
    uint8_t* action) {
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

  auto status_cmd = app.add_subcommand("status", "Status");
  auto j_flag = status_cmd->add_flag("-j,--json");

  CLI11_PARSE(app, argc, argv);

  if (pal_get_fru_id((char *)fru.c_str(), fru_id) != 0) {
    std::cout << "fru is invalid!\n";
    std::cout << app.help() << std::endl;
    return -1;
  }

  if (status_cmd->parsed()) {
    opt_json = (*j_flag) ? true:false;
    *action = BOOT_STATUS;
  } else {
    std::cout << app.help() << std::endl;
    return -1;
  }

  return 0;
}

int main(int argc, char** argv) {
  uint8_t fru_id = 0;
  uint8_t action = UNKNOW_CMD;
  if (parse_args(argc, argv, &fru_id, &action)) {
    return -1;
  }

  switch (action) {
    case BOOT_STATUS:
      do_get_status(fru_id);
      break;
    default:
      return -1;
  }

  return 0;
}
