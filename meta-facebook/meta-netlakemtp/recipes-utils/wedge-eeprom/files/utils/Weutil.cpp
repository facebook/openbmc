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
#include <sstream>

#include "../lib/WeutilInterface.h"

#define USE_NEW_EEPROM_LIB true

using ojson = nlohmann::ordered_json;

static void usage() {
  std::cout
      << "weutil [-h | --help] [-a | --all] [-j | --json] [-f | --format] [-l | --list] [-e <dev-name> | --eeprom <dev-name>]\n";
  std::cout
      << "       [-w <file> | --write <file>] [-m <Type> <Value> | --modify <Type> <Value>]\n";
  std::cout << "   -h | --help : show usage \n";
  std::cout << "   -a | --all : print all eeprom devices data \n";
  std::cout << "   -j | --json : print eeprom data in JSON format \n";
  std::cout << "   -e | --eeprom : eeprom device name \n";
  std::cout << "   -f | --format : eeprom format \n";
  std::cout << "   -l | --list : list all eeprom device names \n";
  std::cout << "   -w | --write : write file data to eeprom \n";
  std::cout << "   -m | --modify : modify type value to eeprom,rules are at the bottom \n";
  std::cout << "        [Type]  |                     [Name]                   |    [Length]\n";
  std::cout << "       -------------------------------------------------------------------------\n";
  std::cout << "          1     |  Product Name                                |   Variable\n";
  std::cout << "          2     |  Product Part Number                         |   Variable\n";
  std::cout << "          3     |  System Assembly Part Number                 |      8\n";
  std::cout << "          4     |  Meta PCBA Part Number                       |      12\n";
  std::cout << "          5     |  Meta PCB Part Number                        |      12\n";
  std::cout << "          6     |  ODM/JDM PCBA Part Number                    |   Variable\n";
  std::cout << "          7     |  ODM/JDM PCBA Serial Number                  |   Variable\n";
  std::cout << "          8     |  Product Production State                    |      1\n";
  std::cout << "          9     |  Product Version                             |      1\n";
  std::cout << "          10    |  Product Sub-Version                         |      1\n";
  std::cout << "          11    |  Product Serial Number                       |   Variable\n";
  std::cout << "          12    |  System Manufacturer                         |   Variable\n";
  std::cout << "          13    |  System Manufacturing Date                   |      8\n";
  std::cout << "          14    |  PCB Manufacturer                            |   Variable\n";
  std::cout << "          15    |  Assembled at                                |   Variable\n";
  std::cout << "          16    |  EEPROM location on Fabric                   |   Variable\n";
  std::cout << "          17    |  X86 CPU (MAC Base + MAC Address Size)       |      8\n";
  std::cout << "          18    |  BMC (MAC Base + BMC MAC Address Size)       |      8\n";
  std::cout << "          19    |  Switch ASIC (MAC Base + MAC Address Size)   |      8\n";
  std::cout << "          20    |  META Reserved (MAC Base + MAC Address Size) |      8\n";
  std::cout << "       -------------------------------------------------------------------------\n";
  std::cout << "          Notice: Type 17~20 need to add - before enter MAC Address Size\n";
  std::cout << "          ex. weutil -m 17 11:22:33:44:55:66-258\n";
}

static void printEepromData(const std::string& eDeviceName, bool jFlag) {
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

std::vector<uint8_t> readFileContents(const std::string& filePath, std::streamsize& fileSize) {
  std::ifstream inputFile(filePath, std::ios::binary | std::ios::ate);
  fileSize = 0; // Initialize file size to 0

  if (inputFile) {
    fileSize = inputFile.tellg();
    if (fileSize > 0) {
      std::vector<uint8_t> data(fileSize);
      inputFile.seekg(0, std::ios::beg);
      inputFile.read(reinterpret_cast<char*>(data.data()), fileSize);
      inputFile.close();
      return data; // Return the read data
    } else {
      std::cerr << "ERROR: File is empty or unable to read file size: " << filePath << std::endl;
      return {};
    }
  } else {
    std::cerr << "ERROR: Unable to open file for reading: " << filePath << std::endl;
    return {};
  }
}

static int writeFileContents(const std::string& filePath, const std::vector<uint8_t>& data) {
  std::ofstream outputFile(filePath, std::ios::binary | std::ios::out);
  if (outputFile) {
    outputFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outputFile.close();
    return 1;
  } else {
    std::cerr << "ERROR: Unable to open file for writing: " << filePath << std::endl;
    return 0;
  }
}

static void writeEepromData(const std::string& eDeviceName, const std::string& eDevicePath, const std::string& fileName) {
    std::streamsize fileSize;
    std::vector<uint8_t> data = readFileContents(fileName, fileSize);

    if (!data.empty()) {
      int result = writeFileContents(eDevicePath, data);
        if (result) {
          std::cout << "Write " << fileName << " to " << eDeviceName << " success !" << std::endl;
          return;
        } else {
          std::cerr << "ERROR: Failed to write " << fileName << " to " << eDeviceName << std::endl;
          return;
        }
    } else {
      std::cerr << "ERROR: Can't write data to " << eDeviceName << std::endl;
      return;
    }
}

static void modifyEepromData(const std::string& eDeviceName,
                             const std::string& eDevicePath,
                             const std::string& modifyType,
                             std::string& modifyValue) {
  int modifyTypeID = std::stoi(modifyType);
  int modifyValueLength = 0;
  std::vector<uint8_t> modifyValueHex;
  std::vector<uint8_t> modifyValueMac;

  // check modify type/length/value
  switch (modifyTypeID) {
    case typeId_v5::ID_SYS_ASSEM_PART_NUM:
      if (modifyValue.length() > SYS_LEN) {
        std::cout << "ERROR: Type " << modifyTypeID << " input value is out of range !" << '\n';
        return;
      } else {
        modifyValueLength = SYS_LEN;
      }
      break;
    case typeId_v5::ID_SYS_MANUFACTURING_DATE:
      if (modifyValue.length() > SYS_MANUFACTURING_DATE_LEN) {
        std::cout << "ERROR: Type " << modifyTypeID << " input value is out of range !" << '\n';
        return;
      } else {
        modifyValueLength = SYS_MANUFACTURING_DATE_LEN;
      }
      break;
    case typeId_v5::ID_PCBA_PART_NUM:
    case typeId_v5::ID_PCB_PART_NUM:
      if (modifyValue.length() > PCB_PCBA_PART_LEN) {
        std::cout << "ERROR: Type " << modifyTypeID << " input value is out of range !" << '\n';
        return;
      } else {
        if (modifyValue.length() < PCB_PCBA_PART_LEN) {
          modifyValue.append(PCB_PCBA_PART_LEN - modifyValue.length(), ' ');
        }
        modifyValueLength = PCB_PCBA_PART_LEN;
      }
      break;
    case typeId_v5::ID_PRODUCT_PRODUCTION_STATE:
    case typeId_v5::ID_PRODUCT_VER:
    case typeId_v5::ID_PRODUCT_SUB_VER:{
        int modifyValueInt = std::stoi(modifyValue);
        if (modifyValueInt < 0 || modifyValueInt > 255) {
          std::cout << "ERROR: Type " << modifyTypeID << " is out of range !" << '\n';
          return;
        } else {
          modifyValueHex.push_back(static_cast<uint8_t>(modifyValueInt));
          modifyValueLength = PRODUCT_VER_STATE_LEN;
        }
      }
      break;
    case typeId_v5::ID_X86_CPU_MAC:
    case typeId_v5::ID_BMC_MAC:
    case typeId_v5::ID_SWITCH_ASIC_MAC:
    case typeId_v5::ID_RSVD_MAC:{
        // check MAC value format
        int colonCount = 0; // detect ":"
        int dashCount = 0;  // detect "-"
        for (char c : modifyValue) {
          if (c == ':') {
            ++colonCount;
          }
          if (c == '-') {
          ++dashCount;
          }
        }

        if ((colonCount != MAC_ADR_LEN) || (dashCount != MAC_ADR_BASE_LEN)) {
          std::cout << "ERROR: Type " << modifyTypeID << " input value format incorrect !" << '\n';
          return;
        } else {// set MAC value format
          std::istringstream modifyValueStream(modifyValue);
          std::string modifyValueSegment;
          while (std::getline(modifyValueStream, modifyValueSegment, ':')) { // set MAC Addresses value
            size_t dashPos = modifyValueSegment.find('-');
            if (dashPos != std::string::npos) {
              modifyValueSegment = modifyValueSegment.substr(0, dashPos);
            }
            modifyValueMac.push_back(static_cast<uint8_t>(std::stoi(modifyValueSegment, nullptr, 16)));
            if (dashPos != std::string::npos) {
              break;
            }
          }

          size_t dashPos = modifyValue.find('-');
          if (dashPos != std::string::npos) { // set MAC Addresses from the Base value
            std::string macBaseStr = modifyValue.substr(dashPos + 1);
            int macBase = std::stoi(macBaseStr);
            modifyValueMac.push_back(static_cast<uint8_t>((macBase >> 8) & 0xFF)); // HighByte
            modifyValueMac.push_back(static_cast<uint8_t>(macBase & 0xFF));        // LowByte
          }
          modifyValueLength = MAC_LEN;
        }
      }
        break;
    default:
      modifyValueLength = modifyValue.length();
      break;
  }

  // read eeprom data value
  std::streamsize fileSize;
  std::vector<uint8_t> data = readFileContents(eDevicePath, fileSize);
  if (data.empty()) {
    std::cerr << "ERROR: Unable to read data from: " << eDeviceName << std::endl;
    return;
  }

  //check eeprom Header
  if (data[0] == HEADER && data[1] == HEADER && data[2] == FORMAT_VERSION) {
    std::cout << eDeviceName << " is follow Meta FBOSS EEPROM Format v5!" << std::endl;
  } else {
    std::cerr << "ERROR: EEPROM type not follow Meta FBOSS EEPROM Format v5! : " << eDeviceName << std::endl;
    return;
  }

  // set the new eeprom data need to follow TLV format
  int eepromType = 4;             // eeprom type address, the fist one is at 4
  int eepromLength = 5;           // eeprom length address, the first one is at 5
  int next_eepromType_adr = 1;    // next type address,   1 means one byte
  int next_eepromLength_adr = 1;  // next length address, 1 means one byte
  int eepromLength_adr_size = 1;  // length address size, 1 means one byte

  std::vector<uint8_t> new_eeprom_data;
  // Check Type is between 1~20
  if (static_cast<int>(data[eepromType]) >= MIN_TYPE && static_cast<int>(data[eepromType]) <= MAX_TYPE) {

    while(1){ //find which type value need to modify

      if ((static_cast<int>(data[eepromType])) == (std::stoi(modifyType))) {

        // add data value to new_eeprom_data before add modify value
        new_eeprom_data.insert(new_eeprom_data.end(), data.begin(), data.begin() + eepromType + 1);

        // insert modify length to new_eeprom_data
        new_eeprom_data.push_back(static_cast<uint8_t>(modifyValueLength));

        switch (modifyTypeID) {
          case typeId_v5::ID_PRODUCT_PRODUCTION_STATE:
          case typeId_v5::ID_PRODUCT_VER:
          case typeId_v5::ID_PRODUCT_SUB_VER:
            new_eeprom_data.insert(new_eeprom_data.end(), modifyValueHex.begin(), modifyValueHex.end());
            break;
          case typeId_v5::ID_X86_CPU_MAC:
          case typeId_v5::ID_BMC_MAC:
          case typeId_v5::ID_SWITCH_ASIC_MAC:
          case typeId_v5::ID_RSVD_MAC:
            new_eeprom_data.insert(new_eeprom_data.end(), modifyValueMac.begin(), modifyValueMac.end());
            break;
          default:
            for (char c : modifyValue) {
              new_eeprom_data.push_back(static_cast<uint8_t>(c));
            }
            break;
        }

        // add data value to new_eeprom_data after add modify value
        if (data.size() > (eepromType + eepromLength_adr_size + (static_cast<int>(data[eepromLength])) + 1)) {
          new_eeprom_data.insert(new_eeprom_data.end(), data.begin() + eepromType + eepromLength_adr_size + (static_cast<int>(data[eepromLength])) + 1, data.end());
        } else {
          std::cerr << "ERROR: modify EEPROM Processing error !" <<std::endl;
          return;
        }
        break;

      } else if (data[eepromType] == NULL_TYPE) { //0xff maens get NULL need to break
        std::cerr << "ERROR: modify EEPROM Processing error get error Type value !" <<std::endl;
        return;
      } else {
        // next_Type_Addr = now_Type_Addr +  Length_Addr + (data)Value_Length_Addr -> +1
        eepromType = eepromType + eepromLength_adr_size + (static_cast<int>(data[eepromLength])) + next_eepromType_adr;

        // next_Length_Addr = now_Length_Addr + (data)Value_Length_Addr + nextType -> +1
        eepromLength = eepromLength + eepromLength_adr_size + (static_cast<int>(data[eepromLength])) + next_eepromLength_adr;
      }
    }

    // calculate new_eeprom_data Header ~ CRC type & length
    size_t CRCendIndex = 0;
    for (int i = new_eeprom_data.size() - 1; i > HEADER_LEN; --i) {
      if ((new_eeprom_data[i-1] == CRC_TYPE_ADR)&&(new_eeprom_data[i] == CRC_LEN_ADR)) {
        CRCendIndex = i;
        break;
      }
    }

    if (CRCendIndex) {
      std::vector<uint8_t> modifyCRCdata(new_eeprom_data.begin(), new_eeprom_data.begin() + CRCendIndex + 1); // get calculate CRC data
      // calculate CRC16(CCITT-FALSE) Checksum and set to new eeprom data
      uint16_t newCRC = CRC_INIT;
      for (auto byte : modifyCRCdata) { // XOR current CRC with byte shifted left
        newCRC ^= static_cast<uint16_t>(byte) << 8;
        for (int i = 0; i < 8; ++i) {
          if (newCRC & CHECK_HIGHEST_BIT) { // Check if the leftmost (highest) bit of newCRC is set 1
            newCRC = (newCRC << 1) ^ TRUNCATED_POLYNOMIA; //If the highest bit is 1, left shift newCRC by one and then XOR with the truncated polynomial
          } else {
            newCRC = newCRC << 1; // If the highest bit is 0, simply left shift newCRC by one
          }
        }
      }
      new_eeprom_data[CRCendIndex + CRC_VALUE_H] = newCRC >> 8;   // CRC HighByte
      new_eeprom_data[CRCendIndex + CRC_VALUE_L] = newCRC & 0xFF; // CRC LowByte
      std::cout << "Calculate new CRC-CCITT value success !" << std::endl;
    } else {
      std::cerr << "ERROR: CRC processing error !" <<std::endl;
      return;
    }

  } else {
    std::cerr << "ERROR: Read "<< eDeviceName << " Type fail!" << std::endl;
  }

  // write new_eeprom_data to eeprom
  if (!new_eeprom_data.empty()) {
    int result = writeFileContents(eDevicePath, new_eeprom_data);
    if (result) {
        std::cout << "Modify value written to " << eDeviceName << " success!" << std::endl;
    } else {
        std::cerr << "ERROR: Failed to write modified data to " << eDeviceName << std::endl;
    }
  } else {
    std::cerr << "ERROR: Modify EEPROM data is empty!" << std::endl;
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
  int writeFlag = 0;
  int modifyFlag = 0;
  int c;
  int modifyTypeInt = 0; //modify value change int
  std::string eepromDeviceName{DEFAULT_EEPROM_NAME};
  std::string writeFileName;
  std::string eepromDevicePath;
  std::string modifyType;
  std::string modifyValue;

  static struct option long_options[] = {
      {"list", no_argument, NULL, 'l'},
      {"format", no_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {"all", no_argument, NULL, 'a'},
      {"json", no_argument, NULL, 'j'},
      {"eeprom", required_argument, NULL, 'e'},
      {"write", required_argument, NULL, 'w'},
      {"modify", required_argument, NULL, 'm'},
      {0, 0, 0, 0}};

  doPrint = (argc == 1) ? 1 : 0;
  while ((c = getopt_long(argc, argv, "ahjlf:e:w:m:", long_options, &opt_index)) !=
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
      case 'w':
        writeFileName = optarg;
        writeFlag = 1;
        break;
      case 'm': {
          if (optind < argc && argv[optind] != NULL && argv[optind][0] != '-') {
            try {
              modifyTypeInt = std::stoi(optarg);
            } catch (const std::invalid_argument& e) {
              std::cerr << "ERROR: Type is Invalid argument: " << optarg << std::endl;
              exit(1);
            } catch (const std::out_of_range& e) {
              std::cerr << "ERROR: Type is Argument out of range: " << optarg << std::endl;
              exit(1);
            }
            modifyType = optarg;
            modifyValue = argv[optind];
            optind++;
            modifyFlag = 1;
          }
          else {
            std::cerr << "ERROR: -m option requires two arguments.\n";
            exit(1);
          }
          break;
        }
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
      std::cout << "EEPROM: " + listEnt.first << std::endl;
      printEepromData(listEnt.first, jsonFlag);
      std::cout << std::endl;
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

  if (writeFlag) {
      if (!writeFileName.empty()) {
        std::map<std::string, std::string> res = listEepromDevices();
        for (auto& eeprom : res) {
          if (eeprom.first == eepromDeviceName) {
            eepromDevicePath = eeprom.second;
          }
        }
        writeEepromData(eepromDeviceName, eepromDevicePath, writeFileName);
      } else {
        std::cout << "ERROR: File is NULL !: " + writeFileName
                  << '\n';
        exit(1);
      }
    exit(0);
  }

  if(modifyFlag) {
    if ((modifyTypeInt >= MIN_TYPE) && (modifyTypeInt <= MAX_TYPE)) {
      //load eeprom device name and path
      std::map<std::string, std::string> res = listEepromDevices();
      for (auto& eeprom : res) {
        if (eeprom.first == eepromDeviceName) {
          eepromDevicePath = eeprom.second;
        }
      }
      modifyEepromData(eepromDeviceName, eepromDevicePath, modifyType, modifyValue);
    } else {
      std::cout << "ERROR: Type is not between 1~20 ! " << '\n';
      exit(1);
    }
    exit(0);
  }
}