#include <hgx.h>
#include <nlohmann/json.hpp>
#include <openbmc/kv.hpp>
#include <string.h>
#include <syslog.h>
#include <chrono>
#include <iostream>
#include <thread>
#include "time_utils.hpp"
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

#include <fstream>
#include <streambuf>

// HTTP response status codes
enum {
  HTTP_OK = 200,
  HTTP_ACCEPTED = 202,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
};

constexpr auto HMC_USR = "root";
constexpr auto HMC_PWD = "0penBmc";

const std::string HMC_BASE_URL = "http://192.168.31.1";
const auto HMC_URL = HMC_BASE_URL + "/redfish/v1/";
const auto HMC_UPDATE_SERVICE = HMC_URL + "UpdateService";
const auto HMC_TASK_SERVICE = HMC_URL + "TaskService/Tasks/";
const auto HMC_FW_INVENTORY = HMC_URL + "UpdateService/FirmwareInventory/";
const auto HGX_TELEMETRY_SERVICE_EVT = HMC_URL +
    "TelemetryService/MetricReportDefinitions/PlatformEnvironmentMetrics";
const auto HGX_TELEMETRY_SERVICE_DVT = HMC_URL +
    "TelemetryService/MetricReports/HGX_PlatformEnvironmentMetrics_0/";
const auto HMC_FACTORY_RESET_SERVICE = "/Actions/Manager.ResetToDefaults";
const auto HMC_RESET_SERVICE = "/Actions/Manager.Reset";
const auto HMC_DUMP_SERVICE = HMC_URL +
    "Managers/HGX_BMC_0/LogServices/Dump/Actions/LogService.CollectDiagnosticData";
const auto SYSTEM_DUMP_SERVICE = HMC_URL +
    "Systems/HGX_Baseboard_0/LogServices/Dump/Actions/LogService.CollectDiagnosticData";

const std::vector<std::string> HMC_PATCH_TARGETS_EVT = {
  "ERoT_FPGA_Firmware", "ERoT_GPU0_Firmware", "ERoT_GPU1_Firmware",
  "ERoT_GPU2_Firmware", "ERoT_GPU3_Firmware", "ERoT_GPU4_Firmware",
  "ERoT_GPU5_Firmware", "ERoT_GPU6_Firmware", "ERoT_GPU7_Firmware",
  "ERoT_NVSwitch0_Firmware", "ERoT_NVSwitch1_Firmware",
  "ERoT_NVSwitch2_Firmware", "ERoT_NVSwitch3_Firmware",
  "ERoT_PCIeSwitch_Firmware", "ERoT_HMC_Firmware", "HMC_Firmware"
};
const std::vector<std::string> HMC_PATCH_TARGETS_DVT = {
  "HGX_FW_ERoT_FPGA_0", "HGX_FW_ERoT_GPU_SXM_1",
  "HGX_FW_ERoT_GPU_SXM_2", "HGX_FW_ERoT_GPU_SXM_3",
  "HGX_FW_ERoT_GPU_SXM_4", "HGX_FW_ERoT_GPU_SXM_5",
  "HGX_FW_ERoT_GPU_SXM_6", "HGX_FW_ERoT_GPU_SXM_7",
  "HGX_FW_ERoT_GPU_SXM_8", "HGX_FW_ERoT_NVSwitch_0",
  "HGX_FW_ERoT_NVSwitch_1", "HGX_FW_ERoT_NVSwitch_2",
  "HGX_FW_ERoT_NVSwitch_3", "HGX_FW_ERoT_PCIeSwitch_0",
  "HGX_FW_ERoT_HMC_0", "HGX_FW_HMC_0"
};

const std::vector<std::string> BMC_PATCH_TARGETS_DVT = {
  "HGX_FW_ERoT_FPGA_0", "HGX_FW_ERoT_GPU_SXM_1",
  "HGX_FW_ERoT_GPU_SXM_2", "HGX_FW_ERoT_GPU_SXM_3",
  "HGX_FW_ERoT_GPU_SXM_4", "HGX_FW_ERoT_GPU_SXM_5",
  "HGX_FW_ERoT_GPU_SXM_6", "HGX_FW_ERoT_GPU_SXM_7",
  "HGX_FW_ERoT_GPU_SXM_8", "HGX_FW_ERoT_NVSwitch_0",
  "HGX_FW_ERoT_NVSwitch_1", "HGX_FW_ERoT_NVSwitch_2",
  "HGX_FW_ERoT_NVSwitch_3", "HGX_FW_ERoT_PCIeSwitch_0",
  "HGX_FW_ERoT_BMC_0", "HGX_FW_BMC_0"
};

constexpr auto TIME_OUT = 6;

using nlohmann::json;

namespace hgx {

class HGXMgr {
 public:
  HGXMgr() {
    RestClient::init();
  }
  ~HGXMgr() {
    RestClient::disable();
  }
  static std::string get(const std::string& url) {
    RestClient::Response result;
    RestClient::Connection conn(url);

    conn.SetTimeout(TIME_OUT);
    conn.SetBasicAuth(HMC_USR, HMC_PWD);

    result = conn.get("");
    if (result.code != HTTP_OK) {
      throw HTTPException(result.code);
    }
    return result.body;
  }

  static std::string post(const std::string& url, std::string&& args, bool isFile) {
    RestClient::Response result;
    RestClient::Connection conn(url);

    if (isFile) {
      std::ifstream t(std::move(args));
      std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
      result =  conn.post("", str);
    }
    else {
      result =  conn.post("", std::move(args));
    }

    if (result.code != HTTP_OK && result.code != HTTP_ACCEPTED) {
      throw HTTPException(result.code);
    }
    return result.body;
  }

  static std::string patch(const std::string& url, std::string&& args) {
    RestClient::Response result;
    RestClient::Connection conn(url);

    result = conn.patch("", std::move(args));

    if (result.code != HTTP_OK && result.code != HTTP_ACCEPTED) {
      throw HTTPException(result.code);
    }
    return result.body;
  }
};

static HGXMgr hgx;

std::string redfishGet(const std::string& subpath) {
  if (subpath.starts_with("/redfish/v1")) {
    return hgx.get(HMC_BASE_URL + subpath);
  }
  return hgx.get(HMC_URL + subpath);
}

std::string redfishPost(const std::string& subpath, std::string&& args) {
  if (subpath.starts_with("/redfish/v1")) {
    return hgx.post(HMC_BASE_URL + subpath, std::move(args), false);
  }
  return hgx.post(HMC_URL + subpath, std::move(args), false);
}

std::string redfishPatch(const std::string& subpath, std::string&& args) {
  if (subpath.starts_with("/redfish/v1")) {
    return hgx.patch(HMC_BASE_URL + subpath, std::move(args));
  }
  return hgx.patch(HMC_URL + subpath, std::move(args));
}

std::string version(const std::string& comp, bool returnJson) {
  std::string url = HMC_FW_INVENTORY + comp;
  std::string respStr = hgx.get(url);
  if (returnJson) {
    return respStr;
  }
  json resp = json::parse(respStr);
  return resp["Version"];
}

std::string updateNonBlocking(const std::string& comp, const std::string& path, bool returnJson) {
  std::string url = HMC_UPDATE_SERVICE;
  std::string fp = path;

  if (comp != "") {
    const std::string targetsPath = "/redfish/v1/UpdateService/FirmwareInventory/";
    json targetJson;
    targetJson["HttpPushUriTargets"] = json::array({targetsPath + comp});
    std::string target = targetJson.dump();
    std::cout << "target: " << target << std::endl;
    hgx.patch(url, std::move(target));
  }

  std::string respStr = hgx.post(url, std::move(fp), true);
  if (returnJson) {
    return respStr;
  }
  json resp = json::parse(respStr);
  return resp["Id"];
}

HMCPhase getHMCPhase() {
  static HMCPhase phase = HMCPhase::HMC_FW_UNKNOWN;
  // TODO Try to do this with one Get at the root.
  auto tryPhase = [](const std::string& url) {
    try {
      hgx.get(url);
      return true;
    } catch (std::exception&) {
      return false;
    }
  };
  if (phase != HMCPhase::HMC_FW_UNKNOWN) {
    return phase;
  }
  if (tryPhase(HMC_FW_INVENTORY + "HGX_FW_BMC_0")) {
    phase = HMCPhase::BMC_FW_DVT;
  } else if (tryPhase(HMC_FW_INVENTORY + "HGX_FW_HMC_0")) {
    phase = HMCPhase::HMC_FW_DVT;
  } else if (tryPhase(HMC_FW_INVENTORY + "HMC_Firmware")) {
    phase = HMCPhase::HMC_FW_EVT;
  }
  return phase;
}

TaskStatus getTaskStatus(const std::string& id) {
  std::string url = HMC_TASK_SERVICE + id;
  TaskStatus status;
  status.resp = hgx.get(url);
  json resp = json::parse(status.resp);
  resp.at("TaskState").get_to(status.state);
  resp.at("TaskStatus").get_to(status.status);
  for (auto& j : resp.at("Messages")) {
    status.messages.emplace_back(j.at("Message"));
  }
  return status;
}

void getMetricReports() {
  std::string url = HGX_TELEMETRY_SERVICE_DVT;
  std::string cache_path = "/tmp/cache_store/";
  std::string snr_val;
  std::string resp;
  unsigned int pos;

  resp = hgx.get(url);
  json jresp = json::parse(resp);
  json &tempArray = jresp["MetricValues"];
  for(auto &x : tempArray)
  {
    auto jname = x.find("MetricProperty");
    std::string snr_path = jname.value();
    pos = snr_path.find_last_of("/\\");
    snr_path = snr_path.substr(pos+1);

    auto jvalue = x.find("MetricValue");
    snr_val = jvalue.value();
    pos = snr_val.find_first_not_of("0123456789");
    if (pos != 0) {
      kv::set(snr_path, snr_val);
    }
    else {
      kv::set(snr_path, "");
    }
  }
}

void factoryReset() {
  static const std::map<HMCPhase, std::string> urlMap = {
    {HMCPhase::HMC_FW_EVT, HMC_URL + "Managers/bmc" + HMC_FACTORY_RESET_SERVICE},
    {HMCPhase::HMC_FW_DVT, HMC_URL + "Managers/HGX_HMC_0" + HMC_FACTORY_RESET_SERVICE},
    {HMCPhase::BMC_FW_DVT, HMC_URL + "Managers/HGX_BMC_0" + HMC_FACTORY_RESET_SERVICE}
  };
  std::string url = urlMap.at(getHMCPhase());
  hgx.post(url, json::object({{"ResetToDefaultsType", "ResetAll"}}).dump(), false);
}

void reset() {
  static const std::map<HMCPhase, std::string> urlMap = {
    {HMCPhase::HMC_FW_EVT, HMC_URL + "Managers/bmc" + HMC_RESET_SERVICE},
    {HMCPhase::HMC_FW_DVT, HMC_URL + "Managers/HGX_HMC_0" + HMC_RESET_SERVICE},
    {HMCPhase::BMC_FW_DVT, HMC_URL + "Managers/HGX_BMC_0" + HMC_RESET_SERVICE}
  };
  std::string url = urlMap.at(getHMCPhase());
  hgx.post(url, json::object({{"ResetType", "GracefulRestart"}}).dump(), false);
}

void patch_bf_update() {
  static const std::map<HMCPhase, const std::vector<std::string>*> patchMap = {
    {HMCPhase::HMC_FW_EVT, &HMC_PATCH_TARGETS_EVT},
    {HMCPhase::HMC_FW_DVT, &HMC_PATCH_TARGETS_DVT},
    {HMCPhase::BMC_FW_DVT, &BMC_PATCH_TARGETS_DVT}
  };
  json patchJson = json::object({{"HttpPushUriTargets", json::array()}});
  for (const auto& target : *patchMap.at(getHMCPhase())) {
    auto& j = patchJson["HttpPushUriTargets"];
    j.push_back("/redfish/v1/UpdateService/FirmwareInventory/" + target);
  }
  hgx.patch(HMC_UPDATE_SERVICE, patchJson.dump());
  auto jresp = json::parse(hgx.get(HMC_UPDATE_SERVICE));
  std::cout << "Patching: " << std::endl \
            << jresp["HttpPushUriTargets"] << std::endl;
}

std::string dumpNonBlocking(DiagnosticDataType type) {
  std::string url;
  const std::map<DiagnosticDataType, std::string> typeMap = {
      {DiagnosticDataType::MANAGER, "Manager"},
      {DiagnosticDataType::OEM_SYSTEM, "System"},
      {DiagnosticDataType::OEM_SELF_TEST, "SelfTest"},
      {DiagnosticDataType::OEM_FPGA, "FPGA"}};
  json req;
  if (type == DiagnosticDataType::MANAGER) {
    req["DiagnosticDataType"] = "Manager";
    url = HMC_DUMP_SERVICE;
  } else {
    url = SYSTEM_DUMP_SERVICE;
    req["DiagnosticDataType"] = "OEM";
    req["OEMDiagnosticDataType"] = "DiagnosticType=" + typeMap.at(type);
  }
  std::string respStr = hgx.post(url, req.dump(), false);
  json resp = json::parse(respStr);
  return resp["Id"];
}

TaskStatus waitTask(const std::string& taskID) {
  using namespace std::chrono_literals;
  size_t nextMessage = 0;
  for (int retry = 0; retry < 500; retry++) {
    TaskStatus status = getTaskStatus(taskID);
    for (; nextMessage < status.messages.size(); nextMessage++) {
      std::cout << status.messages[nextMessage++] << std::endl;
    }
    if (status.state != "Running") {
      return status;
    }
    std::this_thread::sleep_for(5s);
  }
  throw std::runtime_error("Timeout");
}

std::string findTaskPayloadLocation(const TaskStatus& status) {
  json taskResp = json::parse(status.resp);
  for (const std::string hdr : taskResp["Payload"]["HttpHeaders"]) {
    auto sepLoc = hdr.find(": ");
    if (sepLoc == hdr.npos) {
      continue;
    }
    std::string tag = hdr.substr(0, sepLoc);
    std::string val = hdr.substr(sepLoc + 2, hdr.length());
    if (tag == "Location") {
      return val;
    }
  }
  throw std::runtime_error("Header did not contain a location!");
}

void retrieveDump(const std::string& taskID, const std::string& path) {
  using namespace std::chrono_literals;
  TaskStatus status = waitTask(taskID);
  std::string loc = findTaskPayloadLocation(status);
  std::cout << "Task Additional Data: " << loc << std::endl;
  json dumpResp = json::parse(hgx.get(HMC_BASE_URL + loc));
  if (!dumpResp.contains("AdditionalDataURI")) {
    throw std::runtime_error(
        "Task result location does not contain an AdditionalDataURI");
  }
  std::string attachmentURL = dumpResp["AdditionalDataURI"];
  std::cout << "Getting attachment from: " << attachmentURL << std::endl;
  std::string attachment = hgx.get(HMC_BASE_URL + attachmentURL);
  std::ofstream outfile(path, std::ofstream::binary);
  outfile.write(&attachment[0], attachment.size());
}

void dump(DiagnosticDataType type, const std::string& path) {
  std::string taskID = dumpNonBlocking(type);
  retrieveDump(taskID, path);
}

int update(const std::string& comp, const std::string& path) {
  std::string taskID = updateNonBlocking(comp, path);
  std::cout << "Started update task: " << taskID << std::endl;
  TaskStatus status = waitTask(taskID);
  std::cout << "Update completed with state: " << status.state
            << " status: " << status.status << std::endl;
  if (status.state != "Completed") {
    return -1;
  }
  return 0;
}

void printEventLog(std::ostream& os, bool jsonFmt) {
  std::string url =
      HMC_URL + "Systems/HGX_Baseboard_0/LogServices/EventLog/Entries";
  json entries = json::parse(hgx.get(url));
  if (jsonFmt) {
    os << entries["Members"].dump();
    return;
  }
  for (const auto& entry : entries["Members"]) {
    auto& time = entry["Created"];
    auto& msg = entry["Message"];
    auto& resolution = entry["Resolution"];
    bool resolved = entry["Resolved"];
    auto& severity = entry["Severity"];
    auto res_str = resolved ? "RESOLVED" : "UNRESOLVED";
    os << time << '\t' << severity << '\t' << res_str << '\t' << msg << '\t'
       << resolution << std::endl;
  }
}

void syncTime() {
  std::string url = HMC_URL + "Managers/HGX_BMC_0";
  json data = json::object({{"DateTime", hgx::time::now()}});
  hgx.patch(url, data.dump());
}

std::string sensorRaw(const std::string& component, const std::string& name) {
  constexpr auto fru = "Chassis";
  const std::string url = HMC_URL + fru + "/" + component + "/Sensors/" + name;
  return hgx.get(url);
}

float sensor(const std::string& component, const std::string& name) {
  std::string str = sensorRaw(component, name);
  json resp = json::parse(str);
  float val;
  resp.at("Reading").get_to(val);
  return val;
}

} // namespace hgx.

int get_hgx_sensor(const char* component, const char* snr_name, float* value) {
  try {
    *value = hgx::sensor(component, snr_name);
  } catch (std::exception& e) {
    return -1;
  }
  return 0;
}

int get_hgx_ver(const char* component, char *version) {
  try {
    std::string resStr;
    resStr = hgx::version(component, 0);
    strcpy(version, resStr.c_str());
  } catch (std::exception& e) {
    return -1;
  }
  return 0;
}

int hgx_get_metric_reports() {
  try {
    hgx::getMetricReports();
  } catch (std::exception& e) {
      return -1;
  }
  return 0;
}

HMCPhase get_hgx_phase() {
  HMCPhase phase;
  try {
    phase = hgx::getHMCPhase();
  } catch (std::exception& e) {
    phase = HMCPhase::HMC_FW_UNKNOWN;
  }
  return phase;
}
