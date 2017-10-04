#include <iostream>
#include <string>
#include <cstring>
#include <stdio.h>
#include "fw-util.h"
#include <jansson.h>

#define FSC_CONFIG           "/etc/fsc-config.json"

using namespace std;

class FscdComponent : public Component {
  public:
  FscdComponent(string fru, string comp)
    : Component(fru, comp) {}
  int print_version()
  {
    json_error_t error;
    json_t *conf, *vers;

    cout << "Fan Speed Controller Version: ";
    conf = json_load_file(FSC_CONFIG, 0, &error);
    if(!conf) {
      cout << "NA" << endl;
      return FW_STATUS_FAILURE;
    }
    vers = json_object_get(conf, "version");
    if(!vers || !json_is_string(vers)) {
      cout << "NA" << endl;
    } else {
      cout << string(json_string_value(vers)) << endl;
    }
    json_decref(conf);
    return FW_STATUS_SUCCESS;
  }
};

FscdComponent fscd("bmc", "fscd");
