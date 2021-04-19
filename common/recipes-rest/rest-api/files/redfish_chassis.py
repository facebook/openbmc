from subprocess import *
from collections import OrderedDict
from node import node
from node_sensors import sensor_util


def get_chassis():
    body = {}
    try:
        body = {
            "@odata.context":"/redfish/v1/$metadata#ChassisCollection.ChassisCollection",
            "@odata.id": "/redfish/v1/Chassis",
            "@odata.type": "#ChassisCollection.ChassisCollection",
            "Name": "Chassis Collection",
            "Members@odata.count": 1,
            "Members": [{"@odata.id": "/redfish/v1/Chassis/1"}]
        }
    except Exception as error:
        print(error)
    return node(body)


def get_chassis_members(fru_name):
    body = {}
    result = {}
    try:
        cmd = "/usr/local/bin/fruid-util " + fru_name
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
        data = data.decode()
        sdata = data.split("\n")
        for line in sdata:
            # skip lines with --- or startin with FRU
            if line.startswith("FRU"):
                continue
            if line.startswith("-----"):
                continue

            kv = line.split(":", 1)
            if len(kv) < 2:
                continue

            result[kv[0].strip()] = kv[1].strip()

        body = {
            "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
            "@odata.id": "/redfish/v1/Chassis/1",
            "@odata.type": "#Chassis.v1_5_0.Chassis",
            "Id": "1",
            "Name": "Computer System Chassis",
            "ChassisType": "RackMount",
            "Manufacturer": result["Product Manufacturer"],
            "Model": result["Product Name"],
            "SerialNumber": result["Product Serial"],
            "PowerState": "On",
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "Thermal": {
                "@odata.id": "/redfish/v1/Chassis/1/Thermal"
            },
            "Power": {
                "@odata.id": "/redfish/v1/Chassis/1/Power"
            },
            "Links": {
                "ManagedBy": [{"@odata.id": "/redfish/v1/Managers/1"}]
            }}
    except Exception as error:
        print(error)
    return node(body)


def get_chassis_power(fru_name, snr_name, snr_num):
    body = {}
    period = "60"
    display = ["thresholds"]
    try:
        power_limitin = sensor_util(fru_name, snr_name, snr_num, period, display)
        power_limitin = float(power_limitin[str(snr_name)]['thresholds']['UCR'])
        body = {
            "@odata.context": "/redfish/v1/$metadata#Power.Power",
            "@odata.id": "/redfish/v1/Chassis/1/Power",
            "@odata.type": "#Power.v1_5_0.Power",
            "Id": "Power",
            "Name": "Power",
            "PowerControl": [{
                "@odata.id": "/redfish/v1/Chassis/1/Power#/PowerControl/0",
                "MemberId": "0",
                "Name": "System Power Control",
                "PhysicalContext": "Chassis",
                "PowerLimit": {
                    "LimitInWatts": int(power_limitin),
                    "LimitException": "LogEventOnly"
                }
             }]
        }
    except Exception as error:
        print(error)
    return node(body)


def get_chassis_thermal(fru_name):
    body = {}
    result = OrderedDict()
    snr_name = ""
    snr_num = ""
    period = "60"
    display = ["units"]
    temp_sensors = []
    fan_sensors = []
    redundancy_body = []
    try:
        # Temperatures
        temp_count = 0
        temp = sensor_util(fru_name, snr_name, snr_num, period, display)
        for sname, key in temp.items():
            if key['units'] == 'C':
                result = OrderedDict([
                ("@odata.id", "/redfish/v1/Chassis/1/Thermal#/Temperatures/" + str(temp_count)),
                ("MemberId", str(temp_count)),
                ("Name", str(sname)),
                ("ReadingCelsius", float(key['value']))
                ])
                temp_count += 1
                temp_sensors.append(result)

        # Fans
        result = OrderedDict()
        fan_count = 0
        cmd = "/usr/local/bin/fan-util --get"
        data = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
        sdata = data.split("\n")
        for line in sdata:
            # skip lines with " or startin with FRU
            if line.find("Error") != -1:
                continue
            kv = line.split(":")
            if len(kv) < 2:
                continue
            if (kv[0].strip().find("Speed") != -1):
                odata_id_content = "/redfish/v1/Chassis/1/Thermal#/Fans/" + str(fan_count)
                result = OrderedDict([
                ("@odata.id", odata_id_content),
                ("MemberId", str(fan_count)),
                ("Name", "BaseBoard System Fan " + str(fan_count + 1)),
                ("Status", OrderedDict([('State', 'Enabled'), ('Health', 'OK')]))
                ])
                fan_count += 1
                fan_sensors.append(result)
                redundancy_result = OrderedDict([('@odata.id', odata_id_content)])
                redundancy_body.append(redundancy_result)

        body = {
            "@odata.context": "/redfish/v1/$metadata#Thermal.Thermal",
            "@odata.id": "/redfish/v1/Chassis/1/Thermal",
            "@odata.type": "#Thermal.v1_1_0.Thermal",
            "Id": "Thermal",
            "Name": "Thermal",
            "Temperatures": temp_sensors,
            "Fans": fan_sensors,
            "Redundancy": [{
                "@odata.id": "/redfish/v1/Chassis/1/Thermal#/Redundancy/0",
                "MemberId": "0",
                "Name": "BaseBoard System Fans",
                "RedundancyEnabled": bool(0),
                "RedundancySet": redundancy_body,
                "Mode": "N+m",
                "Status": {
                    "State": "Enabled",
                    "Health": "OK"
                },
                "MinNumNeeded": 1
            }]
        }
    except Exception as error:
        print(error)
    return node(body)
