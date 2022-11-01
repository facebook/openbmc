# ",/usr/bin/env python,
# ,
# Copyright 2020-present Facebook. All Rights Reserved.,
# ,
# This program file is free software; you can redistribute it and/or modify it,
# under the terms of the GNU General Public License as published by the,
# Free Software Foundation; version 2 of the License.,
# ,
# This program is distributed in the hope that it will be useful, but WITHOUT,
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or,
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License,
# for more details.,
# ,
# You should have received a copy of the GNU General Public License,
# along with this program in a file named COPYING; if not, write to the,
# Free Software Foundation, Inc.,,
# 51 Franklin Street, Fifth Floor,,
# Boston, MA 02110-1301 USA,
# ,

plat_i2c_tree = {
    "1-0040": {"name": "xdpe132g5c", "driver": "xdpe132g5c"},
    "1-0053": {"name": "mp2978", "driver": "mp2975"},
    "1-0059": {"name": "mp2978", "driver": "mp2975"},
    "2-0035": {"name": "scmcpld", "driver": "scmcpld"},
    "3-0048": {"name": "lm75", "driver": "lm75"},
    "3-0049": {"name": "lm75", "driver": "lm75"},
    "3-004a": {"name": "lm75", "driver": "lm75"},
    "8-0051": {"name": "24c64", "driver": "at24"},
    "8-004a": {"name": "lm75", "driver": "lm75"},
    "12-003e": {"name": "smb_syscpld", "driver": "smb_syscpld"},
    "13-0035": {"name": "iobfpga", "driver": "iobfpga"},
    "17-004c": {"name": "lm75", "driver": "lm75"},
    "17-004d": {"name": "lm75", "driver": "lm75"},
    "19-0052": {"name": "24c64", "driver": "at24"},
    "20-0050": {"name": "24c02", "driver": "at24"},
    "22-0052": {"name": "24c02", "driver": "at24"},
    "28-0050": {"name": "24c02", "driver": "at24"},
    "48-0058": {"name": "psu_driver", "driver": "psu_driver"},
    "49-005a": {"name": "psu_driver", "driver": "psu_driver"},
    "50-004c": {"name": "lm75", "driver": "lm75"},
    "50-0052": {"name": "24c64", "driver": "at24"},
    "51-0048": {"name": "tmp75", "driver": "lm75"},
    "52-0049": {"name": "tmp75", "driver": "lm75"},
    "54-0021": {"name": "pca9534", "driver": "pca953x"},
    "56-0058": {"name": "psu_driver", "driver": "psu_driver"},
    "57-005a": {"name": "psu_driver", "driver": "psu_driver"},
    "59-0048": {"name": "tmp75", "driver": "lm75"},
    "60-0049": {"name": "tmp75", "driver": "lm75"},
    "62-0021": {"name": "pca9534", "driver": "pca953x"},
    "64-0033": {"name": "fcbcpld", "driver": "fcbcpld"},
    "65-0053": {"name": "24c64", "driver": "at24"},
    "66-0048": {"name": "tmp75", "driver": "lm75"},
    "66-0049": {"name": "tmp75", "driver": "lm75"},
    "68-0052": {"name": "24c64", "driver": "at24"},
    "69-0052": {"name": "24c64", "driver": "at24"},
    "70-0052": {"name": "24c64", "driver": "at24"},
    "71-0052": {"name": "24c64", "driver": "at24"},
    "72-0033": {"name": "fcbcpld", "driver": "fcbcpld"},
    "73-0053": {"name": "24c64", "driver": "at24"},
    "74-0048": {"name": "tmp75", "driver": "lm75"},
    "74-0049": {"name": "tmp75", "driver": "lm75"},
    "76-0052": {"name": "24c64", "driver": "at24"},
    "77-0052": {"name": "24c64", "driver": "at24"},
    "78-0052": {"name": "24c64", "driver": "at24"},
    "79-0052": {"name": "24c64", "driver": "at24"},
}


def minipack2_get_pim_i2c_tree(pim):
    if pim < 1 or pim > 8:
        return {}
    bus = 80 + ((pim - 1) * 8)
    list = [
        {"channel": 0, "addr": 0x60, "name": "domfpga", "driver": "domfpga"},
        {"channel": 1, "addr": 0x56, "name": "24c64", "driver": "at24"},
        {"channel": 2, "addr": 0x48, "name": "tmp75", "driver": "lm75"},
        {"channel": 3, "addr": 0x4B, "name": "tmp75", "driver": "lm75"},
        {"channel": 4, "addr": 0x4A, "name": "tmp75", "driver": "lm75"},
        {"channel": 6, "addr": 0x6B, "name": "mp2975", "driver": "mp2975"},
    ]
    devices = {}
    for device in list:
        id = "{}-00{:02x}".format(bus + device["channel"], device["addr"])
        devices[id] = {"name": device["name"], "driver": device["driver"]}
    return devices


minipack2_smb_alternate_i2c = [
    [
        {
            "5-0035": {"name": "ucd90160", "driver": "ucd9000"},
            "5-0036": {"name": "ucd90160", "driver": "ucd9000"},
        },
        {
            "5-0066": {"name": "ucd90160", "driver": "ucd9000"},
            "5-0067": {"name": "ucd90160", "driver": "ucd9000"},
        },
        {
            "5-0068": {"name": "ucd90160", "driver": "ucd9000"},
            "5-0069": {"name": "ucd90160", "driver": "ucd9000"},
        },
        {
            "5-0043": {"name": "ucd9012*", "driver": "ucd9000"},
            "5-0046": {"name": "ucd9012*", "driver": "ucd9000"},
        },
        {
            "5-0044": {"name": "adm1266", "driver": "adm1266"},
            "5-0047": {"name": "adm1266", "driver": "adm1266"},
        },
    ],
    [
        {
            "16-0010": {"name": "adm1278", "driver": "adm1275"},
        },
        {
            "16-0044": {"name": "lm25066", "driver": "lm25066"},
        },
    ],
]


def minipack2_get_pim_alternate_i2c(pim):
    if pim < 1 or pim > 8:
        return []
    bus = 80 + ((pim - 1) * 8)
    list = [
        [
            {"channel": 6, "addr": 0x34, "name": "ucd90160", "driver": "ucd9000"},
            {"channel": 6, "addr": 0x64, "name": "ucd90160", "driver": "ucd9000"},
            {"channel": 6, "addr": 0x65, "name": "ucd90160", "driver": "ucd9000"},
            {"channel": 6, "addr": 0x40, "name": "ucd9012*", "driver": "ucd9000"},
            {"channel": 6, "addr": 0x44, "name": "adm1266", "driver": "adm1266"},
        ],
        [
            {"channel": 4, "addr": 0x10, "name": "adm1278", "driver": "adm1275"},
            {"channel": 4, "addr": 0x44, "name": "lm25066", "driver": "lm25066"},
        ],
    ]
    devices = []
    for alternate_set in list:
        set_array = []
        for alternate in alternate_set:
            id = "{}-00{:02x}".format(bus + alternate["channel"], alternate["addr"])
            set_array.append(
                {id: {"name": alternate["name"], "driver": alternate["driver"]}}
            )
        devices.append(set_array)
    return devices


minipack2_fcmt_alternate_i2c = [
    [
        {"67-0010": {"name": "adm1278", "driver": "adm1275"}},
        {"67-0044": {"name": "lm25066", "driver": "lm25066"}},
    ]
]

minipack2_fcmb_alternate_i2c = [
    [
        {"75-0010": {"name": "adm1278", "driver": "adm1275"}},
        {"75-0044": {"name": "lm25066", "driver": "lm25066"}},
    ]
]

minipack2_smbpowcpld_pdbl_alternate_i2c = [
    [
        {"53-0060": {"name": "smb_pwrcpld", "driver": "smb_pwrcpld"}},
        {"55-0060": {"name": "smb_pwrcpld", "driver": "smb_pwrcpld"}},
    ]
]

minipack2_smbpowcpld_pdbr_alternate_i2c = [
    [
        {"61-0060": {"name": "smb_pwrcpld", "driver": "smb_pwrcpld"}},
        {"63-0060": {"name": "smb_pwrcpld", "driver": "smb_pwrcpld"}},
    ]
]

device_lm75 = {
    "lm75": {"name": "lm75", "driver": "lm75"},
    "tmp75": {"name": "tmp75", "driver": "lm75"},
}