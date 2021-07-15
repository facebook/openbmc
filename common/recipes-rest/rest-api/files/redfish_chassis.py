import typing as t

import redfish_chassis_helper
from aiohttp import web
from common_utils import dumps_bytestr
from redfish_base import validate_keys


async def get_chassis(request: str) -> web.Response:
    body = {
        "@odata.context": "/redfish/v1/$metadata#ChassisCollection.ChassisCollection",
        "@odata.id": "/redfish/v1/Chassis",
        "@odata.type": "#ChassisCollection.ChassisCollection",
        "Name": "Chassis Collection",
        "Members@odata.count": 1,
        "Members": [{"@odata.id": "/redfish/v1/Chassis/1"}],
    }
    await validate_keys(body)
    return web.json_response(body, dumps_bytestr=dumps_bytestr)


async def get_chassis_members(request: str, fru_name: str) -> web.Response:
    # will implement in next diff in the stack
    return web.json_response()


async def get_chassis_power(request: str, fru_name: str) -> web.Response:
    # will implement in next diff in the stack
    return web.json_response()


class ChassisThermal:
    fru_name = None

    def __init__(self, fru_name: t.Optional[str] = None):
        self.fru_name = fru_name
        # if fru_name is None, we retrieve frus for single sled devices

    async def get_chassis_thermal(self, request: str) -> web.Response:
        body = {}
        if redfish_chassis_helper.is_platform_supported():
            temperature_sensors = redfish_chassis_helper.get_temperature_sensors_helper(
                self.fru_name
            )
            temperature_sensors_json = make_temperature_sensors_json_body(
                temperature_sensors
            )
            body = {
                "@odata.type": "#Thermal.v1_7_0.Thermal",
                "@odata.id": "/redfish/v1/Chassis/1/Thermal",
                "Id": "Thermal",
                "Name": "Thermal",
                "Temperatures": temperature_sensors_json,
                "Fans": "",  # will implement in next diffs in the stack
                "Redundancy": "",  # will implement in next diffs in the stack
            }
            await validate_keys(body)
        else:
            raise NotImplementedError("Redfish is not supported in this platform")
        return web.json_response(body, dumps=dumps_bytestr)


def make_temperature_sensors_json_body(
    temperature_sensors: t.List[redfish_chassis_helper.TemperatureSensor],
) -> t.List[t.Dict[str, t.Any]]:
    all_temperature_sensors = []
    for idx, temperature_sensor in enumerate(temperature_sensors):
        sensor_threshold = temperature_sensor.sensor_thresh
        temperature_json = {
            "@odata.id": "/redfish/v1/Chassis/1U/Thermal#/Temperatures/{}".format(idx),
            "MemberId": idx,
            "Name": temperature_sensor.sensor_name,
            "SensorNumber": temperature_sensor.sensor_number,
            "FruName": temperature_sensor.fru_name,
            "Status": {"State": "Enabled", "Health": "OK"},
            "ReadingCelsius": temperature_sensor.reading_celsius,
            "UpperThresholdNonCritical": sensor_threshold.unc_thresh,
            "UpperThresholdCritical": sensor_threshold.ucr_thresh,
            "UpperThresholdFatal": sensor_threshold.unr_thresh,
            "LowerThresholdNonCritical": sensor_threshold.lnc_thresh,
            "LowerThresholdCritical": sensor_threshold.lcr_thresh,
            "LowerThresholdFatal": sensor_threshold.lnr_thresh,
            "PhysicalContext": "Chassis",
        }
        all_temperature_sensors.append(temperature_json)
    return all_temperature_sensors
