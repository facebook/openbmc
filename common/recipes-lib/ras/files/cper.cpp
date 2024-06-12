/*
 * Copyright 2024-present Facebook. All Rights Reserved.
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

#include "cper.hpp"

#include <syslog.h>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unordered_map>

namespace cper
{

namespace fs = std::filesystem;

std::string getValue(const nlohmann::ordered_json& obj, const std::string& key)
{
  if (obj.contains(key))
  {
    const auto& value = obj[key];

    if (value.is_boolean())
    {
      return value.get<bool>() ? "true" : "false";
    }
    else if (value.is_number_integer())
    {
      return std::to_string(value.get<int>());
    } 
    else if (value.is_object())
    {
      return value.dump();
    }
    else if (value.is_string())
    {
      return value.get<std::string>();
    }
  }
  return "NA";
}

int __attribute__((weak)) 
addOemSectionHandlerMap(SectionHandlerMap& sectionHandlerMap)
{
  return CPER_HANDLE_SUCCESS;
}

static int processorSectionHandler(const nlohmann::ordered_json& sectionData, 
                                   std::string& errorMsg)
{
  if (!sectionData.contains("errorInfo"))
  {
    return CPER_HANDLE_FAIL;
  }

  auto& errorInfoArray = sectionData["errorInfo"];
  for (const auto& errorInfoObj: errorInfoArray)
  {
    std::ostringstream oss;
    auto& errorInfo = errorInfoObj["errorInformation"];

    auto errorInfoParams = std::vector<processor_error_info_params_t>{
      {"Transaction Type", "transactionType", "transactionTypeValid"},
      {"Operation", "operation", "operationValid"},
      {"Level", "level", "levelValid"},
      {"Processor Context Corrupt", "processorContextCorrupt", "processorContextCorruptValid"},
      {"Corrected", "corrected", "correctedValid"},
      {"Precise PC", "precisePC", "precisePCValid"},
      {"Restartable PC", "restartablePC", "restartablePCValid"}
    };
    
    oss << "Section Sub-type: " << getValue(errorInfoObj["errorType"], "name");
    for (const auto& param : errorInfoParams)
    {
      if (errorInfo["validationBits"][param.validBit].get<bool>())
      {
        if (errorInfo[param.key].is_object())
        {
          oss << ", " << param.title << ": " 
              << getValue(errorInfo[param.key], "value") 
              << " - " << getValue(errorInfo[param.key], "name");
        }
        else
        {
          oss << ", " << param.title << ": " << getValue(errorInfo, param.key);
        }
      }
      else
      {
        oss << ", " << param.title << ": Not Valid";
      }
    }

    errorMsg = oss.str();
  }

  return CPER_HANDLE_SUCCESS;
}

static int memorySectionHandler(const nlohmann::ordered_json& sectionData, 
                                std::string& errorMsg)
{
  std::ostringstream oss;
  std::string errorType = "NA";
  if (sectionData.contains("memoryErrorType"))
  {
    errorType = getValue(sectionData["memoryErrorType"], "name");
  }

  oss << "Section Sub-type: " << errorType << ", Error Specific: "
      << "Node: " << getValue(sectionData, "node") << " / "
      << "Card: " << getValue(sectionData, "card") << " / "
      << "Module: " << getValue(sectionData, "moduleRank") << " / "
      << "Bank: " << getValue(sectionData["bank"], "value") << " / "
      << "Device: " << getValue(sectionData, "device") << " / "
      << "Row: " << getValue(sectionData, "row") << " / "
      << "Column: " << getValue(sectionData, "column") << " / "
      << "Rank Number: " << getValue(sectionData, "rankNumber");
  
  errorMsg = oss.str();
  
  return CPER_HANDLE_SUCCESS;
}

static int pcieSectionHandler(const nlohmann::ordered_json& sectionData, 
                              std::string& errorMsg)
{
  if (!sectionData.contains("deviceID"))
  {
    return CPER_HANDLE_FAIL;
  }

  std::ostringstream oss;
  auto& devIdObj = sectionData["deviceID"];

  oss << "Error Specific: "
      << "Segment Number: " << getValue(devIdObj, "segmentNumber") << " / "
      << "Bus Number: " << getValue(devIdObj, "primaryOrDeviceBusNumber") << " / "
      << "Secondary Bus Number: " << getValue(devIdObj, "secondaryBusNumber") << " / "
      << "Device Number: " << getValue(devIdObj, "deviceNumber") << " / "
      << "Function Number: " << getValue(devIdObj, "functionNumber");

  errorMsg = oss.str();
  return CPER_HANDLE_SUCCESS;
}

static SectionHandlerMap getSectionHandler()
{
  SectionHandlerMap sectionHandlerMap = {
    {"ARM", processorSectionHandler},
    {"Platform Memory", memorySectionHandler},
    {"PCIe", pcieSectionHandler},
  };

  addOemSectionHandlerMap(sectionHandlerMap);

  return sectionHandlerMap;
}

std::string handleSectionData(const nlohmann::ordered_json& sectionDescriptor, 
                              const nlohmann::ordered_json& sectionData)
{
  std::ostringstream oss;
  std::string errorMsg;

  oss << "Error Type: " << getValue(sectionDescriptor["sectionType"], "type")
      << ", Error Severity: " << getValue(sectionDescriptor["severity"], "name");

  auto sectionHandlerMap = getSectionHandler();
  auto it = sectionHandlerMap.find(sectionDescriptor["sectionType"]["type"]);
  if (it != sectionHandlerMap.end())
  {
    try
    {
      if (it->second(sectionData, errorMsg) != CPER_HANDLE_SUCCESS)
      {
        throw std::runtime_error("Faild to handle section data");
      }
      oss << ", " << errorMsg;
    }
    catch(...)
    {
      oss << ", Invalid Section Data Format";
    }
  }
  else
  {
    oss << ", Unknow Section Information";
  }
  return oss.str();
}

int parseCperFile(const std::string& file, std::vector<std::string>& results)
{
  try
	{
		std::ostringstream cmd;
		auto tempFile = file + ".json";

    // convert CPER raw data to JSON format for further parsing
		cmd << CPER_CONVERT << " to-json " << file << " --out " << tempFile;
		auto rc = std::system(cmd.str().c_str());
		if (WIFEXITED(rc) && (WEXITSTATUS(rc) != 0))
		{
      syslog(LOG_ERR, "Failed to execute %s, rc=%d", cmd.str().c_str(), rc);
      return CPER_HANDLE_FAIL;
		}

		if (!fs::exists(tempFile))
		{
      syslog(LOG_ERR, "CPER json file %s, does not exist", tempFile.c_str());
			return CPER_HANDLE_FAIL;
		}

		std::ifstream ifs(tempFile);
		std::remove(tempFile.c_str());
		nlohmann::ordered_json cperJson;
		ifs >> cperJson;

		if (!cperJson.contains("header") || 
				!cperJson.contains("sectionDescriptors") || 
				!cperJson.contains("sections"))
		{
      syslog(LOG_ERR, "Invalid CPER format found in file %s", tempFile.c_str());
      return CPER_HANDLE_FAIL;
		}
		
		size_t sectionCount = cperJson["header"]["sectionCount"];
    for (size_t i = 0; i < sectionCount; ++i)
		{
			auto& sectionDescriptor = cperJson["sectionDescriptors"][i];
			auto& sectionData = cperJson["sections"][i];
      results.emplace_back(handleSectionData(sectionDescriptor, sectionData));
		}
	}
	catch(const std::exception& e)
	{
		syslog(LOG_ERR, "Failed to parse CPER file: %s, %s", file.c_str(), e.what());
    return CPER_HANDLE_FAIL;
	}

  return CPER_HANDLE_SUCCESS;
}

void limitDumpEntries()
{
  auto begin = fs::directory_iterator(CPER_DUMP_PATH);
  auto end = fs::directory_iterator{};
  auto totalDumps = std::count_if(begin, end, [](const auto& entry) {
      return fs::is_regular_file(entry);
  });

  if (totalDumps < CPER_DUMP_MAX_LIMIT)
  {
    return;
  }

  // get the oldest dumps
  std::ostringstream cmd;
  auto excessDumps = totalDumps - (CPER_DUMP_MAX_LIMIT - 1);
  cmd << "ls -t -1 " << CPER_DUMP_PATH << " | tail -n " << excessDumps;

  auto pipe_close = [](auto fd) { (void)pclose(fd); };
  std::unique_ptr<FILE, decltype(pipe_close)> pipe(
      popen(cmd.str().c_str(), "r"), pipe_close);
  
  if (pipe == nullptr)
  {
    throw std::runtime_error("Failed to run popen function!");
  }

  char buf[256] = {0};
  std::vector<std::string> oldestFiles;
  while (fgets(buf, sizeof(buf), pipe.get()) != nullptr)
  {
    std::string result = buf;
    result.erase(result.find_last_not_of(" \n\r\t") + 1);
    oldestFiles.push_back(result);
  }

  // delete the oldest dumps
  for (const auto& file : oldestFiles) {
    auto filePath = CPER_DUMP_PATH + file;
    try
    {
      fs::remove(filePath);
    }
    catch (const fs::filesystem_error& e)
    {
      syslog(LOG_ERR, "Failed to remove file: %s, %s", filePath.c_str(), e.what());
    }
  }
}

int createCperDumpEntry(const uint8_t& payloadId, 
                        const uint8_t* data, const size_t& dataSize)
{
  if (!fs::exists(CPER_DUMP_PATH))
  {
    if (!fs::create_directories(CPER_DUMP_PATH))
    {
      syslog(LOG_ERR, "Failed to create %s directory", CPER_DUMP_PATH);
      return CPER_HANDLE_FAIL;
    }
  }

  try
  {
    limitDumpEntries();
  }
  catch(const std::exception& e)
  {
    syslog(LOG_ERR, "Failed to limit CPER dump entries. %s.", e.what());
  }

  // use unix timestamp (milliseconds) to generate the CPER raw data filename
  // e.g. slot1_cper_1520857056153
  std::ostringstream oss;
  uint64_t timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch()).count();
  oss << CPER_DUMP_PATH << "slot" << payloadId + 1 << "_cper_" << timestamp;
  auto faultLogFile = fs::path(oss.str());

  std::ofstream ofs;
  ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit);
  try
  {
    ofs.open(faultLogFile);
    ofs.write(reinterpret_cast<const char*>(data), dataSize);
    ofs.close();
  }
  catch (const std::ios_base::failure& e)
  {
    syslog(LOG_ERR, "Failed to save CPER to %s, %s.", 
           faultLogFile.c_str(), e.what());
    return CPER_HANDLE_FAIL;
  }

  try
  {
    std::vector<std::string> entries;
    auto filename = faultLogFile.filename().string();

    if (parseCperFile(faultLogFile, entries) != CPER_HANDLE_SUCCESS)
    {
      throw std::runtime_error("Failed to parse CPER file: " + filename);
    }
    for (const auto& entry: entries)
    {
      syslog(LOG_CRIT, "SEL Entry: FRU: %u, %s, Log: %s", 
						 payloadId + 1, entry.c_str(), filename.c_str());
    }
  }
  catch(const std::exception& e)
  {
    syslog(LOG_ERR, "Failed to create CPER dump entry, %s.", e.what());
    return CPER_HANDLE_FAIL;
  }
  
  return CPER_HANDLE_SUCCESS;
}

} // namespace cper