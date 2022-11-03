/*
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

#include <CLI/CLI.hpp>
#include <openbmc/pal.h>
#include <regex>
#include "exclusion.hpp"
#include "log-util.hpp"
#include "rsyslogd.hpp"

std::string get_fru_list()
{
  char c_fru_list[256] = {0};
  if (pal_get_fru_list(c_fru_list)) {
    return std::string(pal_fru_list);
  }
  return std::string(c_fru_list);
}

int main(int argc, char* argv[]) {
  bool print = false, clear = false, opt_json = false;
  std::set<std::string> fru_list;
  std::string fru_list_str = get_fru_list();
  std::regex pattern(R"(\s*,\s*)");
  std::string fru;

  std::copy(
      std::sregex_token_iterator(
          fru_list_str.begin(), fru_list_str.end(), pattern, -1),
      std::sregex_token_iterator(),
      std::inserter(fru_list, fru_list.begin()));
  std::set<std::string> allowed_fru(fru_list);
  allowed_fru.insert("sys");

  CLI::App app("OpenBMC System Event Log Management Utility");
  app.failure_message(CLI::FailureMessage::help);
  auto actions =
      app.add_option_group("Actions", "Actions possible on the SEL(s)");
  auto print_opt =
      actions->add_flag("--print", print, "Print the SEL for the given FRU");
  actions->add_flag("--clear", clear, "Clear SEL(s) of the given FRU");
  actions->require_option(1);
  app.add_flag("--json", opt_json, "Print SEL(s) in JSON format")
      ->needs(print_opt);
  app.add_option("fru", fru)->check(CLI::IsMember(allowed_fru))->required();

  std::string start_time = "", end_time = "";
  CLI::Option* start_time_opt = app.add_option("-s", start_time, "Starting time for timestamp delete");
  app.add_option("-e", end_time, "Ending time for timestamp delete")->needs(start_time_opt);

  CLI11_PARSE(app, argc, argv);

  std::set<uint8_t> action_fru_set;
  if (fru == "sys") {
    action_fru_set.insert(SELFormat::FRU_SYS);
  } else if (fru == "all") {
    action_fru_set.insert(SELFormat::FRU_ALL);
  } else {
    uint8_t fru_id;
    if (pal_get_fru_id((char*)fru.c_str(), &fru_id)) {
      return -1;
    }
    action_fru_set.insert(fru_id);

    uint8_t pair_fru_id = SELFormat::FRU_ALL;
    bool pair_flag = pal_get_pair_fru(fru_id, &pair_fru_id);
    if (pair_flag && pair_fru_id != SELFormat::FRU_ALL) {
      action_fru_set.insert(pair_fru_id);
    }
  }

  try {
    LogUtil util;
    if (print) {
      if (!start_time.empty() && end_time.empty()) {
          // Grab the end time as current time
          time_t rawtime;
          struct tm* ts;
          std::array<char, 80> curtime{};

          ::time(&rawtime);
          ts = localtime(&rawtime);
          strftime(curtime.data(), curtime.size(), "%Y-%m-%d %H:%M:%S", ts);
          end_time = curtime.data();
      }
      util.print(action_fru_set, start_time, end_time, opt_json);

    } else if (clear) {
      Exclusion guard;
      if (guard.error()) {
        throw std::runtime_error("Could not get lock on log");
      }
      pal_log_clear((char*)fru.c_str());
      if (!start_time.empty() && end_time.empty()) {
          // Grab the end time as current time
          time_t rawtime;
          struct tm* ts;
          std::array<char, 80> curtime{};

          ::time(&rawtime);
          ts = localtime(&rawtime);
          strftime(curtime.data(), curtime.size(), "%Y-%m-%d %H:%M:%S", ts);
          end_time = curtime.data();
      }

      util.clear(action_fru_set, start_time, end_time);
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return -1;
  }
  return 0;
}
