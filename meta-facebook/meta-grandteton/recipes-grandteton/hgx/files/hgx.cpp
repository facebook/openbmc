#include <hgx.h>
#include <nlohmann/json.hpp>
#include <string.h>
#include <chrono>
#include <iostream>
#include <thread>
#include <syslog.h>
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"
#include <openbmc/kv.hpp>

#include <fstream>
#include <streambuf>

// HTTP response status codes
enum {
  HTTP_OK = 200,
  HTTP_ACCEPTED = 202,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
};

enum {
  HMC_FW_EVT = 0,
  HMC_FW_DVT = 1,
  BMC_FW_DVT = 2,
};

constexpr auto HMC_USR = "root";
constexpr auto HMC_PWD = "0penBmc";

const std::string HMC_URL = "http://192.168.31.1/redfish/v1/";
const auto HMC_UPDATE_SERVICE = HMC_URL + "UpdateService";
const auto HMC_TASK_SERVICE = HMC_URL + "TaskService/Tasks/";
const auto HMC_FW_INVENTORY = HMC_URL + "UpdateService/FirmwareInventory/";
const auto HGX_TELEMETRY_SERVICE_EVT = HMC_URL + "TelemetryService/MetricReportDefinitions/PlatformEnvironmentMetrics";
const auto HGX_TELEMETRY_SERVICE_DVT = HMC_URL + "TelemetryService/MetricReports/HGX_PlatformEnvironmentMetrics_0/";
const auto HMC_FACTORY_RESET_SERVICE = "/Actions/Manager.ResetToDefaults";
const auto HMC_RESET_SERVICE = "/Actions/Manager.Reset";

const std::string HMC_PATCH_EVT =
"{\"HttpPushUriTargets\": [\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_FPGA_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU0_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU1_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU2_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU3_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU4_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU5_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU6_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_GPU7_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_NVSwitch0_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_NVSwitch1_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_NVSwitch2_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_NVSwitch3_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_PCIeSwitch_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/ERoT_HMC_Firmware\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HMC_Firmware\"\
]}";

const std::string HMC_PATCH_DVT =
"{\"HttpPushUriTargets\": [\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_FPGA_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_1\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_2\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_3\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_4\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_5\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_6\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_7\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_8\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_1\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_2\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_3\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_PCIeSwitch_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_HMC_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_HMC_0\"\
]}";

const std::string BMC_PATCH_DVT =
"{\"HttpPushUriTargets\": [\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_FPGA_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_1\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_2\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_3\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_4\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_5\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_6\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_7\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_GPU_SXM_8\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_1\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_2\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_NVSwitch_3\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_PCIeSwitch_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_ERoT_BMC_0\",\
\"/redfish/v1/UpdateService/FirmwareInventory/HGX_FW_BMC_0\"\
]}";

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
  return hgx.get(HMC_URL + subpath);
}

std::string redfishPost(const std::string& subpath, std::string&& args) {
  return hgx.post(HMC_URL + subpath, std::move(args), false);
}

std::string redfishPatch(const std::string& subpath, std::string&& args) {
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

static int getHMCPhase() {
  std::string url = "";
  RestClient::Response result;
  RestClient::Connection conn("");

  conn.SetTimeout(TIME_OUT);
  conn.SetBasicAuth(HMC_USR, HMC_PWD);

  url = HMC_FW_INVENTORY + "HGX_FW_BMC_0";
  result = conn.get(url);
  if (result.code == HTTP_OK) {
	return BMC_FW_DVT;
  }

  url = HMC_FW_INVENTORY + "HGX_FW_HMC_0";
  result = conn.get(url);
  if (result.code == HTTP_OK) {
	return HMC_FW_DVT;
  }

  url = HMC_FW_INVENTORY + "HMC_Firmware";
  result = conn.get(url);
  if (result.code == HTTP_OK) {
	return HMC_FW_EVT;
  }

  return -1;
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

int FactoryReset() {
  const std::string RESET_TO_DEFAULTS = "{\"ResetToDefaultsType\": \"ResetAll\"}";
  std::string url = "";
  RestClient::Response result;
  RestClient::Connection conn("");
  json jresp;
  int HMCPhase = -1;

  HMCPhase = getHMCPhase();

  if (HMCPhase == HMC_FW_EVT) {
    url = HMC_URL + "Managers/bmc" + HMC_FACTORY_RESET_SERVICE;
  }
  else if (HMCPhase == HMC_FW_DVT) {
    url = HMC_URL + "Managers/HGX_HMC_0" + HMC_FACTORY_RESET_SERVICE;
  }
  else if (HMCPhase == BMC_FW_DVT) {
    url = HMC_URL + "Managers/HGX_BMC_0" + HMC_FACTORY_RESET_SERVICE;
  }
  else {
    std::cout << "Perform HGX factory reset failed, unknown HMC FW" << std::endl;
    syslog(LOG_CRIT, "Perform HGX factory reset failed, unknown HMC FW");
    return -1;
  }

  conn.SetTimeout(TIME_OUT);
  conn.SetBasicAuth(HMC_USR, HMC_PWD);

  result = conn.post(url, RESET_TO_DEFAULTS);
  if(result.code != HTTP_OK) {
	std::cout << "Perform HGX factory reset failed" << std::endl;
	syslog(LOG_CRIT, "Perform HGX factory reset failed");
   throw HTTPException(result.code);
  }

  std::cout << "Perform HGX factory reset..." << std::endl;
  syslog(LOG_CRIT, "Perform HGX factory reset...");
  return 0;
}

void reset() {
  std::string url;
  int HMCPhase = getHMCPhase();

  if (HMCPhase == HMC_FW_EVT) {
    url = HMC_URL + "Managers/bmc" + HMC_RESET_SERVICE;
  }
  else if (HMCPhase == HMC_FW_DVT) {
    url = HMC_URL + "Managers/HGX_HMC_0" + HMC_RESET_SERVICE;
  }
  else if (HMCPhase == BMC_FW_DVT) {
    url = HMC_URL + "Managers/HGX_BMC_0" + HMC_RESET_SERVICE;
  }
  else {
    std::cout << "Perform HGX factory reset failed, unknown HMC FW" << std::endl;
    syslog(LOG_CRIT, "Perform HGX factory reset failed, unknown HMC FW");
    throw std::runtime_error("Unknown HMC Phase");
  }

  json args;
  args["ResetType"] = "GracefulRestart";
  hgx.post(url, args.dump(), false);
}

int patch_bf_update() {
  std::string url = "";
  RestClient::Response result;
  RestClient::Connection conn("");
  json jresp;
  int HMCPhase = -1;

  HMCPhase = getHMCPhase();

  if (HMCPhase == HMC_FW_EVT) {
    auto data = HMC_PATCH_EVT;
	hgx.patch(HMC_UPDATE_SERVICE, std::move(data));
  }
  else if (HMCPhase == HMC_FW_DVT) {
    auto data = HMC_PATCH_DVT;
    hgx.patch(HMC_UPDATE_SERVICE, std::move(data));
  }
  else if (HMCPhase == BMC_FW_DVT) {
    auto data = BMC_PATCH_DVT;
    hgx.patch(HMC_UPDATE_SERVICE, std::move(data));
  }
  else {
    std::cout << "Patch failed, unknown HMC FW" << std::endl;
    return -1;
  }

  conn.SetTimeout(TIME_OUT);
  conn.SetBasicAuth(HMC_USR, HMC_PWD);

  url = HMC_UPDATE_SERVICE;
  result = conn.get(url);
  if (result.code == HTTP_OK) {
	jresp = json::parse(result.body);
    std::cout << "Patching: " << std::endl \
              << jresp["HttpPushUriTargets"] << std::endl;
  }
  else {
    std::cout << "Patch failed" << std::endl;
    throw HTTPException(result.code);
  }

  return 0;
}

int update(const std::string& comp, const std::string& path) {
  using namespace std::chrono_literals;
  std::string taskID = updateNonBlocking(comp, path);
  std::cout << "Started update task: " << taskID << std::endl;
  size_t nextMessage = 0;
  for (int retry = 0; retry < 500; retry++) {
    TaskStatus status = getTaskStatus(taskID);
    for (; nextMessage < status.messages.size(); nextMessage++) {
      std::cout << status.messages[nextMessage++] << std::endl;
    }
    if (status.state != "Running") {
      std::cout << "Update completed with state: " << status.state
                << " status: " << status.status << std::endl;
      if (status.state == "Completed") {
        return 0;
      }
      else {
        return -1;
      }
    }
    std::this_thread::sleep_for(5s);
  }
  return -1;
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
