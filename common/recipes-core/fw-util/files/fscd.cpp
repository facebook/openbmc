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
  int print_version()
  {
    cout << "Fan Speed Controller Version: " << get_fsc_ver_str() << endl;;
    return FW_STATUS_SUCCESS;
  }

  void get_ver_in_json(json& j) {
    j["VERSION"] = get_fsc_ver_str();
    return;
  }
};

FscdComponent fscd("bmc", "fscd");
