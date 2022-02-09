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

def get_chassis_power(power_list):
    body = {}
    result = OrderedDict()
    period = 60
    display = ["thresholds"]
    history = ["history"]
    power_control = []
    try:
        for power in power_list:
            fru_name,snr_name,snr_num = power
            memId = power_list.index(power)

            power_history = sensor_util(fru_name, snr_name, snr_num, str(period), history)
            power_limitin = sensor_util(fru_name, snr_name, snr_num, str(period), display)
            power_limitin = float(power_limitin[snr_name]['thresholds']['UCR'])

            power_metrix = OrderedDict([
            ("IntervalInMin", int(period/60)),
            ("MinIntervalConsumedWatts", float(power_history[snr_name]['min'])),
            ("MaxIntervalConsumedWatts", float(power_history[snr_name]['max'])),
            ("AverageIntervalConsumedWatts", float(power_history[snr_name]['avg']))
            ])

            power_limit = OrderedDict([
            ("LimitInWatts", int(power_limitin)),
            ("LimitException", "LogEventOnly")
            ])

            result = OrderedDict([
            ("@odata.id", f"/redfish/v1/Chassis/1/Power#/PowerControl/{memId}"),
            ("MemberId", str(memId)),
            ("Name", f"System Power Control {memId}"),
            ("PowerLimit", power_limit),
            ("PowerMetrix", power_metrix)
            ])
            power_control.append(result)

        body = {
            "@odata.context": "/redfish/v1/$metadata#Power.Power",
            "@odata.id": "/redfish/v1/Chassis/1/Power",
            "@odata.type": "#Power.v1_5_0.Power",
            "Id": "Power",
            "Name": "Power",
            "PowerControl": power_control
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
    display = ["units","thresholds", "status"]
    temp_sensors = []
    fan_sensors = []
    redundancy_body = []
    thr_type = ['LCR', 'LNR', 'LNC', 'UCR', 'UNR', 'UNC']
    thr_name = ['LowerThresholdCritical', 'LowerThresholdFatal', 'LowerThresholdFatal',
                'UpperThresholdCritical', 'UpperThresholdFatal', 'UpperThresholdNonCritical']
    try:
        # Temperatures, Fans
        temp_count = 0
        fan_count = 0
        temp = sensor_util(fru_name, snr_name, snr_num, period, display)
        for sname, key in temp.items():
            if key['units'] == 'C':
                result = OrderedDict([
                ("@odata.id", "/redfish/v1/Chassis/1/Thermal#/Temperatures/" + str(temp_count)),
                ("MemberId", str(temp_count)),
                ("Name", str(sname)),
                ("ReadingCelsius", float(key['value']))
                ])
                if 'thresholds' in key:
                    s_thresholds = key['thresholds']
                    for i in range(len(thr_name)):
                        if thr_type[i] in s_thresholds:
                            result[thr_name[i]] = float(s_thresholds[thr_type[i]])
                temp_count += 1
                temp_sensors.append(result)
            elif key['units'] == 'RPM':
                if (key['status']  == "ok"):
                    state = 'Enabled'
                    health = 'OK'
                else:
                    state = 'Disabled'
                    health = 'Warning'
                odata_id_content = "/redfish/v1/Chassis/1/Thermal#/Fans/" + str(fan_count)
                result = OrderedDict([
                ("@odata.id", odata_id_content),
                ("MemberId", str(fan_count)),
                ("Name", "BaseBoard System Fan " + str(fan_count + 1)),
                ("Status", OrderedDict([('State', state), ('Health', health)])),
                ("Reading", float(key['value'])),
                ("ReadingUnits", "RPM")
                ])

                if 'thresholds' in key:
                    s_thresholds = key['thresholds']
                    for i in range(len(thr_name)):
                        if thr_type[i] in s_thresholds:
                            result[thr_name[i]] = float(s_thresholds[thr_type[i]])

                fan_fru = int(fan_count / 2 + 1)
                cmd = "/usr/local/bin/fruid-util fan" + str(fan_fru)
                data = Popen(cmd, shell=True, stdout=PIPE).stdout.read()
                data = data.decode('utf-8', 'ignore')
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

                    if "Manufacturer" in line:
                        result["Manufacturer"] = kv[1].strip()
                    if "Serial" in line:
                        result["SerialNumber"] = kv[1].strip()
                    if "Part Number" in line:
                        result["PartNumber"] = kv[1].strip()

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
