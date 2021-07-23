import typing as t

import redfish_chassis_helper
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


class RedfishChassis:
    fru_name = None

    def __init__(self, fru_name: t.Optional[str] = None):
        self.fru_name = fru_name
        # if fru_name is None, we retrieve frus for single sled devices

    def get_server_name(self):
        "Returns the server name for an fru that their routes include"
        server_name = "1"  # default for single sled frus
        if self.fru_name is not None:
            #  replace slot with server
            server_name = self.fru_name.replace("slot", "server")
        return server_name

    async def get_chassis(self, request: str) -> web.Response:
        members_json = redfish_chassis_helper.get_chassis_members_json()
        body = {
            "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Chassis",
            "@odata.type": "#ChassisCollection.ChassisCollection",
            "Name": "Chassis Collection",
            "Members@odata.count": 1,
            "Members": members_json,
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_members(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        if not redfish_chassis_helper.is_platform_supported():
            raise NotImplementedError("Redfish is not supported in this platform")

        frus_info_list = await redfish_chassis_helper.get_fru_info_helper(self.fru_name)
        fru_info_json = make_fru_info_json_body(frus_info_list)
        body = {
            "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
            "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
            "@odata.type": "#Chassis.v1_5_0.Chassis",
            "Id": "1",
            "Name": "Computer System Chassis",
            "ChassisType": "RackMount",
            "FruInfo": fru_info_json,
            "PowerState": "On",
            "Status": {"State": "Enabled", "Health": "OK"},
            "Thermal": {
                "@odata.id": "/redfish/v1/Chassis/{}/Thermal".format(server_name)
            },
            "Power": {"@odata.id": "/redfish/v1/Chassis/{}/Power".format(server_name)},
            "Links": {},
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_thermal(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        body = {}
        if not redfish_chassis_helper.is_platform_supported():
            raise NotImplementedError("Redfish is not supported in this platform")

        temperature_sensors = redfish_chassis_helper.get_sensor_details_helper(
            ["C"], self.fru_name
        )
        temperature_sensors_json = make_temperature_sensors_json_body(
            temperature_sensors, server_name
        )
        fan_sensors = redfish_chassis_helper.get_sensor_details_helper(
            ["RPM", "%"], self.fru_name
        )
        redundancy_list = []
        fan_sensors_json = make_fan_sensors_json_body(
            fan_sensors, redundancy_list, server_name
        )
        body = {
            "@odata.type": "#Thermal.v1_7_0.Thermal",
            "@odata.id": "/redfish/v1/Chassis/{}/Thermal".format(server_name),
            "Id": "Thermal",
            "Name": "Thermal",
            "Temperatures": temperature_sensors_json,
            "Fans": fan_sensors_json,
            "Redundancy": [
                {
                    "@odata.id": "/redfish/v1/Chassis/{}/Thermal#/Redundancy/0".format(  # noqa: B950
                        server_name
                    ),
                    "MemberId": "0",
                    "Name": "BaseBoard System Fans",
                    "RedundancySet": redundancy_list,
                    "Mode": "N+m",
                    "Status": {"State": "Enabled", "Health": "OK"},
                }
            ],
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_power(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        body = {}
        if not redfish_chassis_helper.is_platform_supported():
            raise NotImplementedError("Redfish is not supported in this platform")

        power_control_sensors = redfish_chassis_helper.get_sensor_details_helper(
            ["Amps", "Watts", "Volts"], self.fru_name
        )
        power_control_sensors_json = make_power_sensors_json_body(
            power_control_sensors, server_name
        )
        body = {
            "@odata.context": "/redfish/v1/$metadata#Power.Power",
            "@odata.id": "/redfish/v1/Chassis/{}/Power".format(server_name),
            "@odata.type": "#Power.v1_5_0.Power",
            "Id": "Power",
            "Name": "Power",
            "PowerControl": power_control_sensors_json,
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)


def make_fru_info_json_body(
    frus_info_list: t.List[redfish_chassis_helper.FruInfo],
) -> t.List[t.Dict[str, t.Any]]:
    all_frus_details = []
    for fru in frus_info_list:
        fru_details_json = {
            "FruName": fru.fru_name,
            "Manufacturer": fru.manufacturer,
            "Model": fru.model,
            "SerialNumber": fru.serial_number,
        }
        all_frus_details.append(fru_details_json)
    return all_frus_details


def make_temperature_sensors_json_body(
    temperature_sensors: t.List[redfish_chassis_helper.SensorDetails], server_name: str
) -> t.List[t.Dict[str, t.Any]]:
    all_temperature_sensors = []
    for idx, temperature_sensor in enumerate(temperature_sensors):
        sensor_threshold = temperature_sensor.sensor_thresh
        temperature_json = {
            "@odata.id": "/redfish/v1/Chassis/{server_name}/Thermal#/Temperatures/{idx}".format(  # noqa: B950
                server_name=server_name, idx=idx
            ),
            "MemberId": idx,
            "Name": temperature_sensor.sensor_name,
            "SensorNumber": temperature_sensor.sensor_number,
            "FruName": temperature_sensor.fru_name,
            "Status": {"State": "Enabled", "Health": "OK"},
            "ReadingCelsius": temperature_sensor.reading,
            "UpperThresholdNonCritical": round(sensor_threshold.unc_thresh, 2),
            "UpperThresholdCritical": round(sensor_threshold.ucr_thresh, 2),
            "UpperThresholdFatal": round(sensor_threshold.unr_thresh, 2),
            "LowerThresholdNonCritical": round(sensor_threshold.lnc_thresh, 2),
            "LowerThresholdCritical": round(sensor_threshold.lcr_thresh, 2),
            "LowerThresholdFatal": round(sensor_threshold.lnr_thresh, 2),
            "PhysicalContext": "Chassis",
        }
        all_temperature_sensors.append(temperature_json)
    return all_temperature_sensors


def make_fan_sensors_json_body(
    fan_sensors: t.List[redfish_chassis_helper.SensorDetails],
    redundancy_list: t.List[t.Dict[str, t.Any]],
    server_name: str,
) -> t.List[t.Dict[str, t.Any]]:
    all_fan_sensors = []
    for idx, fan_sensor in enumerate(fan_sensors):
        sensor_threshold = fan_sensor.sensor_thresh
        fan_json = {
            "@odata.id": "/redfish/v1/Chassis/{server_name}/Thermal#/Fans/{idx}".format(
                server_name=server_name, idx=idx
            ),
            "MemberId": idx,
            "Name": fan_sensor.sensor_name,
            "PhysicalContext": "Backplane",
            "SensorNumber": fan_sensor.sensor_number,
            "FruName": fan_sensor.fru_name,
            "Status": {"State": "Enabled", "Health": "OK"},
            "Reading": fan_sensor.reading,
            "ReadingUnits": fan_sensor.sensor_unit,
            "LowerThresholdFatal": round(sensor_threshold.lnr_thresh, 2),
            "Redundancy": [
                {
                    "@odata.id": "/redfish/v1/Chassis/{server_name}/Thermal#/Redundancy/{idx}".format(  # noqa: B950
                        server_name=server_name, idx=idx
                    )
                },
            ],
        }
        all_fan_sensors.append(fan_json)
        redundancy_list.append({"@odata.id": fan_json["@odata.id"]})
    return all_fan_sensors


def make_power_sensors_json_body(
    power_sensors: t.List[redfish_chassis_helper.SensorDetails], server_name: str
) -> t.List[t.Dict[str, t.Any]]:
    all_power_sensors = []
    period = 60
    for idx, power_sensor in enumerate(power_sensors):
        sensor_threshold = power_sensor.sensor_thresh
        _sensor_history = power_sensor.sensor_history
        power_json = [
            {
                "@odata.id": "/redfish/v1/Chassis/{server_name}/Power#/PowerControl/{idx}".format(  # noqa: B950
                    server_name=server_name, idx=idx
                ),
                "MemberId": idx,
                "Name": power_sensor.sensor_name,
                "SensorNumber": power_sensor.sensor_number,
                "PhysicalContext": "Chassis",
                "FruName": power_sensor.fru_name,
                "PowerLimit": {
                    "LimitInWatts": round(sensor_threshold.ucr_thresh, 2),
                    "LimitException": "LogEventOnly",
                },
                "PowerMetrix": {
                    "IntervalInMin": period / 60,
                    "MinIntervalConsumedWatts": round(
                        _sensor_history.min_intv_consumed, 2
                    ),
                    "MaxIntervalConsumedWatts": round(
                        _sensor_history.max_intv_consumed, 2
                    ),
                    "AverageIntervalConsumedWatts": round(
                        _sensor_history.avg_intv_consumed, 2
                    ),
                },
            }
        ]
        all_power_sensors.append(power_json)
    return all_power_sensors
