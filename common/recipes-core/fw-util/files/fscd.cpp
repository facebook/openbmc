#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdio.h>
#include "fw-util.h"

using namespace std;

constexpr auto FSC_CONFIG = "/etc/fsc-config.json";

class FscdComponent : public Component {
  private:
  string get_fsc_ver_str() {
    json j(nullptr);
    ifstream ifs(FSC_CONFIG);
    string ver;

    if (!ifs) {
      return "NA";
    }

    try {
      j = json::parse(ifs);
      //access existing value and rely on default value
      ver = j.value("version", "error_returned");
    } catch (json::parse_error& e) {
      ver = "error_returned";
      cout << "parse_error: " << e.what() << endl;
    }
    return ver;
  }

  public:
  FscdComponent(string fru, string comp)
    : Component(fru, comp) {}
  int get_version(json& j) override {
    j["PRETTY_COMPONENT"] = "Fan Speed Controller";
    j["VERSION"] = get_fsc_ver_str();
    return FW_STATUS_SUCCESS;
  }
};

FscdComponent fscd("bmc", "fscd");
