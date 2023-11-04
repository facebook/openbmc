/*
 * Copyright 2023-present Meta Platform. All Rights Reserved.
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
#include <getopt.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#include "../lib/WeutilInterface.h"

#define USE_NEW_EEPROM_LIB true

using ojson = nlohmann::ordered_json;

static void usage() {
  std::cout
      << "weutil [-h | --help] [-a | --all] [-j | --json] [-f | --format] [-l | --list] [-e <dev-name> | --eeprom <dev-name>]\n";
  std::cout << "   -h | --help : show usage \n";
  std::cout << "   -a | --all : print all eeprom devices data \n";
  std::cout << "   -j | --json : print eeprom data in JSON format \n";
  std::cout << "   -e | --eeprom : eeprom device name \n";
  std::cout << "   -f | --format : eeprom format \n";
  std::cout << "   -l | --list : list all eeprom device names \n";
}

static void printEepromData(const std::string& eDeviceName, bool jFlag) {
  std::cout << "Wedge EEPROM " + eDeviceName << "\n";

  if (USE_NEW_EEPROM_LIB) {
      std::vector<std::pair<std::string, std::string>> parsedData = eepromParseNew(eDeviceName);
      if (jFlag) {
        ojson j;
        for (auto item : parsedData) {
          j[item.first] = item.second;
        }
        std::cout << std::setw(4) << j << '\n';
      } else {
        for (auto item : parsedData) {
          std::cout << item.first << ": " << item.second << '\n';
        }
      }
  } else {
      std::map<fieldId, std::pair<std::string, std::string>> devTbl =
          eepromParse(eDeviceName);
      if (jFlag) {
        ojson j;
        for (auto& item : devTbl) {
          j[item.second.first] = item.second.second;
        }
        std::cout << std::setw(4) << j << '\n';
      } else {
        for (auto& item : devTbl) {
          std::cout << item.second.first << ": " << item.second.second << '\n';
        }
      }
  }
  return;
}

int main(int argc, char* argv[]) {
  int jsonFlag = 0;
  int listFlag = 0;
  int formatFlag = 0;
  int helpFlag = 0;
  int doPrint = 0;
  int opt_index = 0;
  int allFlag = 0;
  int c;
  std::string eepromDeviceName{DEFAULT_EEPROM_NAME};

  static struct option long_options[] = {
      {"list", no_argument, NULL, 'l'},
      {"format", no_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {"all", no_argument, NULL, 'a'},
      {"json", no_argument, NULL, 'j'},
      {"eeprom", required_argument, NULL, 'e'},
      {0, 0, 0, 0}};

  doPrint = (argc == 1) ? 1 : 0;
  while ((c = getopt_long(argc, argv, "ahjlfe:", long_options, &opt_index)) !=
         -1) {
    switch (c) {
      case 'e':
        eepromDeviceName = optarg;
        doPrint = 1;
        break;
      case 'h':
        helpFlag = 1;
        break;
      case 'f':
        formatFlag = 1;
        break;
      case 'a':
        allFlag = 1;
        doPrint = 1;
        break;
      case 'l':
        listFlag = 1;
        break;
      case 'j':
        jsonFlag = 1;
        doPrint = 1;
        break;
      case ':': // missing option argument.
        std::cout << argv[0] << "option -" << optopt
                  << " requires an argument\n";
        break;
      case '?':
      default: // invalid option
        std::cout << argv[0] << "option -" << optopt
                  << " is invalid: ignored\n";
        exit(1);
    }
  }

  if (helpFlag) {
    usage();
    exit(0);
  }

  if (formatFlag) {
    std::cout << eepromFormat(eepromDeviceName) << '\n';
    exit(0);
  }

  if (listFlag) {
    std::map<std::string, std::string> res = listEepromDevices();
    for (auto& item : res) {
      std::cout << item.first << "    " << item.second << "\n";
    }
    exit(0);
  }

  if (allFlag) {
    std::map<std::string, std::string> res = listEepromDevices();
    for (auto listEnt : res) {
      printEepromData(listEnt.first, jsonFlag);
    }
    exit(0);
  }

  if (doPrint) {
    if (eepromNameToPath(eepromDeviceName).value_or("") == "") {
      std::cout << "ERROR: invalid eeprom device name: " + eepromDeviceName
                << '\n';
      exit(1);
    }
    printEepromData(eepromDeviceName, jsonFlag);
  }
}
