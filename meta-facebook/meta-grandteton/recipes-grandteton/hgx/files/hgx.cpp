#include <hgx.h>
#include <nlohmann/json.hpp>
#include <string.h>
#include <chrono>
#include <iostream>
#include <thread>
#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"

// HTTP response status codes
enum {
  HTTP_OK = 200,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
};

constexpr auto HMC_USR = "root";
constexpr auto HMC_PWD = "0penBmc";

const std::string HMC_URL = "http://192.168.31.1/redfish/v1/";
const auto HMC_UPDATE_SERVICE = HMC_URL + "UpdateService";
const auto HMC_TASK_SERVICE = HMC_URL + "TaskService/Tasks/";
const auto HMC_FW_INVENTORY = HMC_URL + "UpdateService/FirmwareInventory/";

constexpr auto TIME_OUT = 2;

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

  static std::string post(const std::string& url, std::string&& args) {
    RestClient::Response result;
    RestClient::Connection conn(url);

    conn.SetTimeout(TIME_OUT);
    conn.SetBasicAuth(HMC_USR, HMC_PWD);

    result = conn.post("", std::move(args));
    if (result.code != HTTP_OK) {
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
  return hgx.post(HMC_URL + subpath, std::move(args));
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

std::string updateNonBlocking(const std::string& path, bool returnJson) {
  std::string url = HMC_UPDATE_SERVICE;
  std::string fp = "{" + path + "}";
  std::string respStr = hgx.post(url, std::move(fp));
  if (returnJson) {
    return respStr;
  }
  json resp = json::parse(respStr);
  return resp["Id"];
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

void update(const std::string& path) {
  using namespace std::chrono_literals;
  std::string taskID = updateNonBlocking(path);
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
      break;
    }
    std::this_thread::sleep_for(5s);
  }
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

int get_hmc_sensor(const char* component, const char* snr_name, float* value) {
  try {
    *value = hgx::sensor(component, snr_name);
  } catch (std::exception& e) {
    return -1;
  }
  return 0;
}

int get_hmc_ver(const char* component, char *version) {
  try {
    std::string resStr;
    resStr = hgx::version(component, 0);
    strcpy(version, resStr.c_str());
  } catch (std::exception& e) {
    return -1;
  }
  return 0;
}
