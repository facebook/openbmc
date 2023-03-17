#!/usr/bin/env python
# -*- coding: utf-8 -*-

import kv

from bios_plat_info import is_halfdome


def cpu_package_power_limit(fru, argv):
    option = argv[2]

    if is_halfdome():
        print("Currently the platform does not support cpu_package_power_limit")
        pass
    else:
        if option == "status":
            try:
                result = kv.kv_get(
                        "fru" + str(fru) + "_power_limit_status", kv.FPERSIST, True
                        )
                if (int(result, 16) & 1) == 1:
                    print("Power limit status: enable")
                else:
                    print("Power limit status: disable")
            except kv.KeyNotFoundFailure as e:
                print("Power limit status: unknown")
        elif option == "enable":
            result = kv.kv_set(
                    "fru" + str(fru) + "_power_limit_status", "0x81", kv.FPERSIST
                    )
            print("Done. Need a power cycle to take the power-limit-enable effect")
        elif option == "disable":
            result = kv.kv_set(
                    "fru" + str(fru) + "_power_limit_status", "0x80", kv.FPERSIST
                    )
            print("Done. Need a power cycle to take the power-limit-disable effect")
        else:
            print("Invalid argument " + str(option))
            return False

    return True
