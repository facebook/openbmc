// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "CrcLib.h"
#include "FbossEepromParser.h"

namespace {

auto constexpr kMaxEepromSize = 2048;
const std::optional<int> VARIABLE = std::nullopt;

enum entryType {
  FIELD_INVALID,
  FIELD_LE_UINT,
  FIELD_BE_UINT,
  FIELD_LE_HEX,
  FIELD_BE_HEX,
  FIELD_STRING,
  FIELD_V5_MAC
};

typedef struct {
  int typeCode;
  std::string fieldName;
  entryType fieldType;
  std::optional<int> length;
  std::optional<int> offset;
} EepromFieldEntry;

std::vector<EepromFieldEntry> kFieldDictionaryV5 = {
    {0, "NA", FIELD_LE_UINT, -1, -1}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE},
    {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {8, "Product Production State", FIELD_BE_UINT, 1, VARIABLE},
    {9, "Product Version", FIELD_BE_UINT, 1, VARIABLE},
    {10, "Product Sub-Version", FIELD_BE_UINT, 1, VARIABLE},
    {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE},
    {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {15, "Assembled At", FIELD_STRING, VARIABLE, VARIABLE},
    {16, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE},
    {17, "X86 CPU MAC", FIELD_V5_MAC, 8, VARIABLE},
    {18, "BMC MAC", FIELD_V5_MAC, 8, VARIABLE},
    {19, "Switch ASIC MAC", FIELD_V5_MAC, 8, VARIABLE},
    {20, "META Reserved MAC", FIELD_V5_MAC, 8, VARIABLE},
    {250, "CRC16", FIELD_BE_HEX, 2, VARIABLE},
};

const std::vector<EepromFieldEntry> kFieldDictionaryV6 = {
    {0, "NA", FIELD_LE_UINT, -1, -1}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE},
    {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {8, "Production State", FIELD_BE_UINT, 1, VARIABLE},
    {9, "Production Sub-State", FIELD_BE_UINT, 1, VARIABLE},
    {10, "Re-Spin/Variant Indicator", FIELD_BE_UINT, 1, VARIABLE},
    {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE},
    {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {15, "Assembled At", FIELD_STRING, VARIABLE, VARIABLE},
    {16, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE},
    {17, "X86 CPU MAC", FIELD_V5_MAC, 8, VARIABLE},
    {18, "BMC MAC", FIELD_V5_MAC, 8, VARIABLE},
    {19, "Switch ASIC MAC", FIELD_V5_MAC, 8, VARIABLE},
    {20, "META Reserved MAC", FIELD_V5_MAC, 8, VARIABLE},
    {21, "RMA", FIELD_BE_UINT, 1, VARIABLE},
    {101, "Vendor Defined Field 1", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {102, "Vendor Defined Field 2", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {103, "Vendor Defined Field 3", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {250, "CRC16", FIELD_BE_HEX, 2, VARIABLE},
};

std::vector<EepromFieldEntry> getEepromFieldDict(int version) {
  switch (version) {
    case 5:
      return kFieldDictionaryV5;
    case 6:
      return kFieldDictionaryV6;
    default:
      throw std::runtime_error(
          "Invalid EEPROM version : " + std::to_string(version));
      break;
  };
  // The control should not come here, but adding this default
  // return value to avoid compiler warning.
  return kFieldDictionaryV5;
};

std::string parseMacHelper(int len, unsigned char* ptr, bool useBigEndian) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  int juice = 0;
  while (juice < len) {
    unsigned int val = useBigEndian ? ptr[juice] : ptr[len - juice - 1];
    std::ostringstream ss;
    ss << std::hex << val;
    std::string strElement = ss.str();
    // Pad 0 if the hex value is only 1 digit. Also,
    // add ':' between 2 hex digits except for the last element
    strElement =
        (val < 16 ? "0" : "") + strElement + (juice != len - 1 ? ":" : "");
    retVal += strElement;
    juice = juice + 1;
  }
  return retVal;
}

} // namespace

namespace facebook::fboss::platform {

std::vector<std::pair<std::string, std::string>>
FbossEepromParser::getContents() {
  unsigned char buffer[kMaxEepromSize + 1] = {};

  int readCount = loadEeprom(eepromPath_, buffer, offset_, kMaxEepromSize);

  std::unordered_map<int, std::string> parsedValue;
  int eepromVer = buffer[2];
  switch (eepromVer) {
    case 5:
    case 6:
      parsedValue = parseEepromBlobTLV(
          eepromVer, buffer, std::min(readCount, kMaxEepromSize));
      break;
    default:
      throw std::runtime_error(
          "EEPROM version is not supported. Only ver 5+ is supported.");
      break;
  }

  return prepareEepromFieldMap(parsedValue, eepromVer);
}

/*
 * Helper function, given the eeprom path, read it and store the blob
 * to the char array output
 */
int FbossEepromParser::loadEeprom(
    const std::string& eeprom,
    unsigned char* output,
    int offset,
    int max) {
  // Declare buffer, and fill it up with 0s
  int fileSize = 0;
  int bytesToRead = max;
  std::ifstream file(eeprom, std::ios::binary);
  int readCount = 0;
  // First, detect EEPROM size, upto 2048B only
  try {
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    // bytesToRead cannot be bigger than the remaining bytes of the file from
    // the offset. That is, we cannot read beyond the end of the file.
    // If the remaining bytes are smaller than max, then we only read up to
    // the end of the file.
    int remainingBytes = fileSize - offset;
    if (bytesToRead > remainingBytes) {
      bytesToRead = remainingBytes;
    }
  } catch (std::exception& ex) {
    std::cout << "Failed to detect EEPROM size (" << eeprom
              << "): " << ex.what() << std::endl;
    throw std::runtime_error("Unabled to detect EEPROM size.");
  }
  if (fileSize < 0) {
    std::cout << "EEPROM (" << eeprom << ") does not exist, or is empty!"
              << std::endl;
    throw std::runtime_error("Unable to read EEPROM.");
  }
  // Now, read the eeprom
  try {
    file.seekg(offset, std::ios::beg);
    file.read((char*)&output[0], bytesToRead);
    readCount = (int)file.gcount();
    file.close();
  } catch (std::exception& ex) {
    std::cout << "Failed to read EEPROM contents " << ex.what() << std::endl;
    readCount = 0;
  }
  return readCount;
}

// Helper function of getInfo, for V5 eeprom and newer
std::unordered_map<int, std::string> FbossEepromParser::parseEepromBlobTLV(
    int eepromVer,
    const unsigned char* buffer,
    const int readCount) {
  int juice = 0; // A variable to count the number of items
                 // parsed so far
  int cursor = 4; // According to the Meta EEPROM v4 spec and later,
                  // the actual data starts from 4th byte of eeprom.
  std::unordered_map<int, std::string> parsedValue;
  std::string value;

  std::vector<EepromFieldEntry> fieldDictionary = getEepromFieldDict(eepromVer);

  while (cursor < readCount) {
    // Increment the item counter (mainly for debugging purposes)
    // Very important to do this.
    juice = juice + 1;
    // First, get the itemCode of the TLV (T)
    int itemCode = (int)buffer[cursor];
    entryType itemType = FIELD_INVALID;
    int itemLen = (int)buffer[cursor + 1];
    std::string key;

    // Vendors pad EEPROM with 0xff. Therefore, if item code is
    // 0xff, then we reached to the end of the actual content.
    if (itemCode == 0xFF) {
      break;
    }
    // Look up our table to find the itemType and field name of this itemCode
    for (size_t i = 0; i < fieldDictionary.size(); i++) {
      if (fieldDictionary[i].typeCode == itemCode) {
        itemType = fieldDictionary[i].fieldType;
        key = fieldDictionary[i].fieldName;
      }
    }
    // If no entry found, throw an exception
    if (itemType == FIELD_INVALID) {
      std::cout << " Unknown field code " << itemCode << " at position "
                << cursor << " item number " << juice << std::endl;
      throw std::runtime_error(
          "Invalid field code in EEPROM at :" + std::to_string(cursor));
    }

    // Find Length and Variable (L and V)
    int itemLength = buffer[cursor + 1];
    unsigned char* itemDataPtr = (unsigned char*)&buffer[cursor + 2];
    // Parse the value according to the itemType
    switch (itemType) {
      case FIELD_LE_UINT:
        value = parseLeUint(itemLength, itemDataPtr);
        break;
      case FIELD_BE_UINT:
        value = parseBeUint(itemLength, itemDataPtr);
        break;
      case FIELD_LE_HEX:
        value = parseLeHex(itemLength, itemDataPtr);
        break;
      case FIELD_BE_HEX:
        value = parseBeHex(itemLength, itemDataPtr);
        break;
      case FIELD_STRING:
        value = parseString(itemLength, itemDataPtr);
        break;
      case FIELD_V5_MAC:
        value = parseV5Mac(itemLength, itemDataPtr);
        break;
      default:
        std::cout << " Unknown field type " << itemType << " at position "
                  << cursor << " item number " << juice << std::endl;
        throw std::runtime_error("Invalid field type in EEPROM.");
        break;
    }
    // Add the key-value pair to the result
    parsedValue[itemCode] = value;
    // Increment the cursor
    cursor += itemLen + 2;
    // the CRC16 is the last content, parsing must stop.
    if (key == "CRC16") {
      uint16_t crcProgrammed = std::stoi(value, nullptr, 16);
      uint16_t crcCalculated =
          weutil::CrcLib::crc16_ccitt_aug(buffer, cursor - 4);
      if (crcProgrammed == crcCalculated) {
        parsedValue[itemCode] += " (CRC Matched)";
      } else {
        std::stringstream ss;
        ss << " (CRC Mismatch. Expected 0x" << std::hex << crcCalculated << ")";
        parsedValue[itemCode] += ss.str();
      }
      break;
    }
  }
  return parsedValue;
}

// Another helper function of getInfo
// This method will translate <field_id, value> pair into
// <field_name, value> pair, so as to be used in other
// methods to print the human readable information
std::vector<std::pair<std::string, std::string>>
FbossEepromParser::prepareEepromFieldMap(
    std::unordered_map<int, std::string> parsedValue,
    int eepromVer) {
  std::vector<std::pair<std::string, std::string>> result;
  std::vector<EepromFieldEntry> fieldDictionary;
  fieldDictionary = getEepromFieldDict(eepromVer);

  // Add the EEPROM version to parsed result. It's not part of the
  // field dictionary, so we add it here.
  result.push_back({"Version", std::to_string(eepromVer)});

  for (auto dictItem : fieldDictionary) {
    std::string key = dictItem.fieldName;
    std::string value;
    auto match = parsedValue.find(dictItem.typeCode);
    // "NA" is reserved, and not for display
    if (key == "NA") {
      continue;
    }
    if (dictItem.fieldType == FIELD_V5_MAC) {
      // MAC V5 field is composite field. One field expands to two items,
      // which are "Base" and "Address Size"
      if (match != parsedValue.end()) {
        std::string key1 = key + " Base";
        std::string key2 = key + " Address Size";
        value = parsedValue.find(dictItem.typeCode)->second;
        // Now unpack this into value1 and value2, delimited by ","
        std::string value1, value2;
        size_t pos = value.find(",");
        if (pos != std::string::npos) {
          value1 = value.substr(0, pos);
          value2 = value.substr(pos + 1);
        } else {
          // Something is wrong. There should be a delimiter.
          throw std::runtime_error("MAC V5 parsing Error. No delimiter found.");
        }
        // From V5 EEPROM Spec, MAC V5 fileds are optional; some MAC field
        // may show up in some eeprom, and some not. Therefore, we add the
        // items only when the key is present.
        result.push_back({key1, value1});
        result.push_back({key2, value2});
      }
    } else {
      // Regular Field (one item ==> one entry)
      if (match != parsedValue.end()) {
        value = parsedValue.find(dictItem.typeCode)->second;
      } else {
        value = "";
      }
      result.push_back({key, value});
    }
  }
  return result;
}

// Parse Little Endian Uint field
std::string FbossEepromParser::parseLeUint(int len, const unsigned char* ptr) {
  // For now, we only support up to 4 Bytes of data
  if (len > 4) {
    throw std::runtime_error("Unsigned int can be up to 4 bytes only.");
  }
  unsigned int readVal = 0;
  // Values in the EEPROM is big endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[cursor];
    cursor -= 1;
  }
  return std::to_string(readVal);
}

// Parse Big Endian Uint field
std::string FbossEepromParser::parseBeUint(int len, const unsigned char* ptr) {
  // For now, we only support up to 4 Bytes of data
  if (len > 4) {
    throw std::runtime_error("Unsigned int can be up to 4 bytes only.");
  }
  unsigned int readVal = 0;
  // Values in the EEPROM is big endian
  // Thus cursor starts from the end and goes backwards
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[i];
  }
  return std::to_string(readVal);
}

std::string FbossEepromParser::parseLeHex(int len, const unsigned char* ptr) {
  std::string retVal = "";
  // Values in the EEPROM is Little endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    int val = ptr[cursor];
    std::string converter = "0123456789abcdef";
    retVal = retVal + converter[(int)(val / 16)] + converter[val % 16];
    cursor -= 1;
  }
  return "0x" + retVal;
}

std::string FbossEepromParser::parseBeHex(int len, const unsigned char* ptr) {
  std::string retVal = "";
  // Values in the EEPROM is big endian
  for (int i = 0; i < len; i++) {
    int val = ptr[i];
    std::string converter = "0123456789abcdef";
    retVal = retVal + converter[(int)(val / 16)] + converter[val % 16];
  }
  return "0x" + retVal;
}

// Parse String field
std::string FbossEepromParser::parseString(int len, const unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  int juice = 0;
  while ((juice < len) && (ptr[juice] != 0)) {
    retVal += (ptr[juice]);
    juice = juice + 1;
  }
  return retVal;
}

// For EEPROM V5, Parse MAC with the format XX:XX:XX:XX:XX:XX, along with two
// bytes MAC size
std::string FbossEepromParser::parseV5Mac(int len, unsigned char* ptr) {
  std::string retVal = "";
  // Pack two string with "," in between. This will be unpacked in the
  // dump functions.
  retVal =
      parseMacHelper(len - 2, ptr, true) + "," + parseBeUint(2, &ptr[len - 2]);
  return retVal;
}

} // namespace facebook::fboss::platform
