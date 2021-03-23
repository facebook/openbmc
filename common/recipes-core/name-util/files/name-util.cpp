/*
 * name-util
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

#include <string>
#include <vector>
#include <unordered_set>
#include <nlohmann/json.hpp>
#include <CLI/CLI.hpp>
#include <openbmc/pal.h>
#include "name.hpp"

using json = nlohmann::json;

const std::string version = "1.0.0";

const std::string help = "Name Utility\n"
                         "Shows all possible FRU names on the system\n"
                         "(Not all FRUs may be present)\n";

bool FruNames::is_present(uint8_t fru)
{
  uint8_t status;
  if (pal_is_fru_prsnt(fru, &status) == 0 && status != 0) {
    return true;
  }
  return false;
}
int FruNames::get_capability(uint8_t fru, unsigned int *caps)
{
  return pal_get_fru_capability(fru, caps);
}
int FruNames::get_fru_name(uint8_t fru, std::string& name)
{
  char cname[64];
  if (pal_get_fru_name(fru, cname)) {
    return -1;
  }
  name.assign(cname);
  return 0;
}
int FruNames::get_list(const std::string& fru_type, std::ostream& os)
{
  std::vector<std::string> bmc;
  std::vector<std::string> nic;
  std::vector<std::string> server;
  for (uint8_t fru = 1; fru <= MAX_NUM_FRUS; fru++) {
    unsigned int caps;
    if (!is_present(fru))
      continue;
    if (get_capability(fru, &caps)) {
      std::cerr << "Could not get FRU: " << (int)fru << " capability!\n";
      continue;
    }
    std::string fru_name;
    if (get_fru_name(fru, fru_name)) {
      std::cerr << "Could not get name for FRU: " << int(fru) << "\n";
      continue;
    }
    if ((caps & FRU_CAPABILITY_SERVER)) {
      server.push_back(fru_name);
    }
    if ((caps & FRU_CAPABILITY_NETWORK_CARD)) {
      nic.push_back(fru_name);
    }
    if ((caps & FRU_CAPABILITY_MANAGEMENT_CONTROLLER)) {
      bmc.push_back(fru_name);
    }
  }

  json j;
  j["version"] = version;
  if (fru_type == "server_fru" || fru_type == "all") {
    j["server_fru"] = server;
  }
  if (fru_type == "bmc_fru" || fru_type == "all") {
    j["bmc_fru"] = bmc;
  }
  if (fru_type == "nic_fru" || fru_type == "all") {
    j["nic_fru"] = nic; 
  }
  os << j.dump(4) << std::endl;
  return 0;
}

int main(int argc, char **argv)
{
  CLI::App app(help);
  app.failure_message(CLI::FailureMessage::help);
  std::unordered_set<std::string> allowed_types{"all", "bmc_fru", "nic_fru", "server_fru"};
  std::string fru_type;
  app.add_option("fru_type", fru_type, "Type of FRU")
    ->required()
    ->check(CLI::IsMember(allowed_types));
  CLI11_PARSE(app, argc, argv);
  FruNames impl;

  return impl.get_list(fru_type, std::cout);
}
