#include "restclient-cpp/connection.h"
#include "restclient-cpp/restclient.h"
#include <iostream>
#include <string.h>
#include <nlohmann/json.hpp>
#include <hgx.h>

//HTTP response status codes
enum {
  HTTP_OK = 200,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND   = 404,
};

constexpr auto HMC_USR            = "root";
constexpr auto HMC_PWD            = "0penBmc";

const std::string HMC_URL            = "http://192.168.31.1/redfish/v1/";
const auto HMC_UPDATE_SERVICE = HMC_URL + "UpdateService";
const auto HMC_TASK_SERVICE   = HMC_URL + "TaskService/Tasks/";
const auto HMC_FW_INVENTORY   = HMC_URL + "UpdateService/FirmwareInventory/";

constexpr auto TIME_OUT = 2;

using nlohmann::json;

class HGXMgr {
  public:
    HGXMgr() {
      RestClient::init();
    }
    ~HGXMgr() {
      RestClient::disable();
    }
    static int get_request (const std::string& url, std::string &res) {
      RestClient::Response result;
      RestClient::Connection conn(url);

      conn.SetTimeout(TIME_OUT);
      conn.SetBasicAuth(HMC_USR, HMC_PWD);

      result = conn.get("");

      if (result.code !=  HTTP_OK) {
        return result.code;
      }
      res = std::move(result.body);
      return HTTP_OK;
    }

    static int post_request (const std::string& url, const std::string&& args, std::string &res) {
      RestClient::Response result;
      RestClient::Connection conn(url);

      conn.SetTimeout(TIME_OUT);
      conn.SetBasicAuth(HMC_USR, HMC_PWD);

      result = conn.post("", args);
      if (result.code !=  HTTP_OK) {
        return result.code;
      }
      res = std::move(result.body);
      return HTTP_OK;
    }
};

static HGXMgr hgx;

int get_version (const std::string& comp, std::string &res) {
  int ret = HTTP_OK;
  std::string url = HMC_FW_INVENTORY + comp;

  ret = hgx.get_request(url, res);

  if (ret != HTTP_OK) {
    return -1;
  }

  return 0;
}

int update (const std::string& path, std::string &res) {
  std::string url = HMC_UPDATE_SERVICE;
  std::string fp;

  fp = "{" + path + "}";
  if (hgx.post_request(url, std::move(fp), res) != HTTP_OK) {
    return -1;
  }

  return 0;
}

int get_taskid (const std::string& task_id, std::string &res) {
  int ret = HTTP_OK;
  std::string url = HMC_TASK_SERVICE + task_id;

  ret = hgx.get_request(url, res);

  if (ret != HTTP_OK) {
    return -1;
  }

  return 0;
}

int get_hmc_sensor (const char* component , const char* snr_name, float* value) {
  int ret = HTTP_OK;
  const std::string comp(component);
  const std::string name(snr_name);
  constexpr auto fru = "Chassis";
  const std::string url = HMC_URL + fru + "/" + comp + "/Sensors/" + name;
  std::string res;

  ret = hgx.get_request(url, res);
  if (ret != HTTP_OK) {
    return -1;
  }

  try {
    json val = json::parse(res);
    val.at("Reading").get_to(*value);
  } catch (std::exception& e) {
    return -1;
  }
  
  return 0;
}
