import json
import os.path

DEFAULT_FSC_VERSION = "fbgc-evt-default-v1"
DEFAULT_FSC_FILE = "FSC_GC_EVT_default_zone0.fsc"

DEFAULT_FAN_TABLE = "/etc/FSC_GC_EVT_default_config.json"
TYPE5_FAN_TABLE = "/etc/FSC_GC_Type5_EVT_v1_config.json"

if os.path.isfile(TYPE5_FAN_TABLE):
    with open(TYPE5_FAN_TABLE, "r") as f:
        fsc_config = json.load(f)

        fsc_config["version"] = DEFAULT_FSC_VERSION
        fsc_config["zones"]["zone_0"]["expr_file"] = DEFAULT_FSC_FILE

        # remove E1.S sensor monitoring and validation checking
        if "E1S_(.*)0_TEMP" in fsc_config["sensor_valid_check"]:
            del fsc_config["sensor_valid_check"]["E1S_(.*)0_TEMP"]
        if "E1S_(.*)1_TEMP" in fsc_config["sensor_valid_check"]:
            del fsc_config["sensor_valid_check"]["E1S_(.*)1_TEMP"]
        if "linear_e1s_sensor_temp" in fsc_config["profiles"]:
            del fsc_config["profiles"]["linear_e1s_sensor_temp"]

        with open(DEFAULT_FAN_TABLE, "w") as outfile:
            json.dump(fsc_config, outfile)
