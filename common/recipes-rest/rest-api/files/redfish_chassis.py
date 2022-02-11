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
            "Members@odata.count": len(members_json),
            "Members": members_json,
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_members(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        frus_info_list = await redfish_chassis_helper.get_fru_info_helper(self.fru_name)
        fru = frus_info_list[0]
        body = {
            "@odata.context": "/redfish/v1/$metadata#Chassis.Chassis",
            "@odata.id": "/redfish/v1/Chassis/{}".format(server_name),
            "@odata.type": "#Chassis.v1_15_0.Chassis",
            "Id": "1",
            "Name": "Computer System Chassis",
            "ChassisType": "RackMount",
            "PowerState": "On",
            "Manufacturer": fru.manufacturer,
            "Model": fru.model,
            "SerialNumber": fru.serial_number,
            "Status": {"State": "Enabled", "Health": "OK"},
            "Sensors": {
                "@odata.id": "/redfish/v1/Chassis/{}/Sensors".format(server_name)
            },
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
        if redfish_chassis_helper.is_libpal_supported():  # for compute and
            # newer fboss platforms that support libpal
            temperature_sensors = (
                redfish_chassis_helper.get_sensor_details_using_libpal_helper(
                    ["C"], self.fru_name
                )
            )
            fan_sensors = redfish_chassis_helper.get_sensor_details_using_libpal_helper(
                ["RPM", "Percent"], self.fru_name
            )
        else:  # for older fboss platforms that don't support libpal
            temperature_sensors = (
                redfish_chassis_helper.get_older_fboss_sensor_details_filtered(
                    "BMC", [redfish_chassis_helper.LIB_SENSOR_TEMPERATURE]
                )
            )
            fan_sensors = (
                redfish_chassis_helper.get_older_fboss_sensor_details_filtered(
                    "BMC", [redfish_chassis_helper.LIB_SENSOR_FAN]
                )
            )
        # Get aggregate sensors and add at the end of fan sensors list
        fan_sensors.extend(redfish_chassis_helper.get_aggregate_sensors())

        redundancy_list = []
        temperature_sensors_json = make_temperature_sensors_json_body(
            temperature_sensors, server_name
        )
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
                    "MinNumNeeded": 1,
                    "Status": {"State": "Enabled", "Health": "OK"},
                }
            ],
        }
        await validate_keys(body)
        return web.json_response(body, dumps=dumps_bytestr)

    async def get_chassis_power(self, request: str) -> web.Response:
        server_name = self.get_server_name()
        body = {}
        # for compute and new fboss platforms
        if redfish_chassis_helper.is_libpal_supported():
            power_control_sensors = (
                redfish_chassis_helper.get_sensor_details_using_libpal_helper(
                    ["Amps", "Watts", "Volts"], self.fru_name
                )
            )
        else:  # for older fboss platforms
            power_control_sensors = (
                redfish_chassis_helper.get_older_fboss_sensor_details_filtered(
                    "BMC",
                    [
                        redfish_chassis_helper.LIB_SENSOR_POWER,
                        redfish_chassis_helper.LIB_SENSOR_IN,
                        redfish_chassis_helper.LIB_SENSOR_CURR,
                    ],
                )
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
            "MemberId": str(idx),
            "Name": temperature_sensor.sensor_name,
            "SensorNumber": temperature_sensor.sensor_number,
            "Status": {"State": "Enabled", "Health": "OK"},
            "ReadingCelsius": int(temperature_sensor.reading),
            "PhysicalContext": "Chassis",
        }
        status = {
            "Status": {"State": "Enabled", "Health": "OK"}
        }  # default unless we have bad reading value

        if sensor_threshold is None:
            threshold_json = {}
        else:
            threshold_json = {
                "UpperThresholdNonCritical": int(sensor_threshold.unc_thresh),
                "UpperThresholdCritical": int(sensor_threshold.ucr_thresh),
                "UpperThresholdFatal": int(sensor_threshold.unr_thresh),
                "LowerThresholdNonCritical": int(sensor_threshold.lnc_thresh),
                "LowerThresholdCritical": int(sensor_threshold.lcr_thresh),
                "LowerThresholdFatal": int(sensor_threshold.lnr_thresh),
            }

        # in case of a bad reading value, update status
        if temperature_sensor.reading == redfish_chassis_helper.SAD_SENSOR:
            status = {"Status": {"State": "UnavailableOffline", "Health": "Critical"}}

        temperature_json.update(threshold_json)
        temperature_json.update(status)
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
            "MemberId": str(idx),
            "Name": fan_sensor.sensor_name,
            "PhysicalContext": "Backplane",
            "SensorNumber": fan_sensor.sensor_number,
            "Status": {"State": "Enabled", "Health": "OK"},
            "Reading": int(fan_sensor.reading),
            # removed ReadingUnits because we were hardcoding it for
            # older fboss platforms that could be incorrect. If we
            # can get correct units from sensors.py, we can add it back.
            "Redundancy": [
                {
                    "@odata.id": "/redfish/v1/Chassis/{server_name}/Thermal#/Redundancy/0".format(  # noqa: B950
                        server_name=server_name
                    )
                },
            ],
        }

        # default status unless we have bad reading value
        status = {"Status": {"State": "Enabled", "Health": "OK"}}
        if sensor_threshold is None:  # for older fboss platforms
            # placeholder lower thresh bc sensors.py doesn't provide this
            fan_json["LowerThresholdFatal"] = 0
        else:  # for compute and new fboss platforms
            fan_json["LowerThresholdFatal"] = int(sensor_threshold.lnr_thresh)

        # in case of a bad reading value, update status
        if fan_sensor.reading == redfish_chassis_helper.SAD_SENSOR:
            status = {"Status": {"State": "UnavailableOffline", "Health": "Critical"}}
        fan_json.update(status)
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
        # default status unless we have bad reading value
        status_val = {"State": "Enabled", "Health": "OK"}
        if (
            power_sensor.reading != redfish_chassis_helper.SAD_SENSOR
            and redfish_chassis_helper.is_libpal_supported()
        ):  # for compute and new fboss
            min_interval_watts = round(_sensor_history.min_intv_consumed, 2)
            max_interval_watts = round(_sensor_history.max_intv_consumed, 2)
            avg_interval_watts = round(_sensor_history.avg_intv_consumed, 2)
            limit_in_watts = round(sensor_threshold.ucr_thresh, 2)
        else:  # for older fboss platforms or unhealthy sensors
            pw_reading = power_sensor.reading
            if pw_reading == redfish_chassis_helper.SAD_SENSOR or pw_reading < 0:
                pw_reading = 0

            min_interval_watts = pw_reading
            max_interval_watts = pw_reading
            avg_interval_watts = pw_reading
            limit_in_watts = power_sensor.sensor_thresh.ucr_thresh
            if limit_in_watts < 0:
                # Redfish schema doesn't allow values < 0 for limit_in_watts.
                # So, if its a negative, default it to 0.
                limit_in_watts = 0

        # in case of a bad reading value, update status
        if power_sensor.reading == redfish_chassis_helper.SAD_SENSOR:
            status_val = {"State": "UnavailableOffline", "Health": "Critical"}
        power_json_item = {
            "@odata.id": "/redfish/v1/Chassis/{server_name}/Power#/PowerControl/{idx}".format(  # noqa: B950
                server_name=server_name, idx=idx
            ),
            "MemberId": str(idx),
            "Name": power_sensor.sensor_name,
            "PhysicalContext": "Chassis",
            "PowerMetrics": {
                "IntervalInMin": int(period / 60),
                "MinConsumedWatts": int(min_interval_watts),
                "MaxConsumedWatts": int(max_interval_watts),
                "AverageConsumedWatts": int(avg_interval_watts),
            },
            "PowerLimit": {
                "LimitInWatts": int(limit_in_watts),
                "LimitException": "LogEventOnly",
            },
            "Status": status_val,
        }
        all_power_sensors.append(power_json_item)
    return all_power_sensors
