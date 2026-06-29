#!/usr/bin/env python
# -*- coding: utf-8 -*-

import kv

EXPECTED_ARG_COUNT = 3

def cpu_package_power_limit(fru, argv):
    if len(argv) != EXPECTED_ARG_COUNT:
        print("Error: Invalid number of arguments.")
        return False

    key_name = "server_power_limit_status"
    option = argv[2]

    if option == "status":
        try:
            result = kv.kv_get(
                key_name, kv.FPERSIST, False
            )
            print(f"Power limit status: {result}")
        except kv.KeyNotFoundFailure as e:
            print("Power limit status: uninitialized")
        except Exception as e:
            print(f"Error: Failed to get status due to {e}")
            return False
    elif option in ["enable", "disable"]:
        try:
            kv.kv_set(key_name, option, kv.FPERSIST)
            msg = f"Done. Need a power cycle to take the power-limit-{option} effect"
            print(msg)
        except Exception as e:
            print(f"Error: Failed to save configuration: {e}")
            return False
    else:
        print("Invalid argument " + str(option))
        return False

    return True
