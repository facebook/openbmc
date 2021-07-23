import json
import os
import time
import typing as t
from shutil import which

import pal
import sdr
from aiohttp.log import server_logger
from common_utils import async_exec


SensorDetails = t.NamedTuple(
    "SensorDetails",
    [
        ("sensor_name", str),
        ("sensor_number", int),
        ("fru_name", str),
        ("reading", str),
        #  Not referencing sdr.ThreshSensor as sdr is unavailable on unit test env
        ("sensor_thresh", "sdr.ThreshSensor"),
        ("sensor_unit", str),
        ("sensor_history", "pal.SensorHistory")
        #  Not referencing pal.SensorHistory as pal is unavailable on unit test env
    ],
)

FRUID_UTIL_PATH = "/usr/local/bin/fruid-util"
WEUTIL_PATH = "/usr/local/bin/weutil"


def get_sensor_details(
    fru_name: str, desired_sensor_units: t.List[str]
) -> t.List[SensorDetails]:
    fru_name_map = pal.pal_fru_name_map()
    fru_id = fru_name_map[fru_name]
    sensor_details_list = []
    if pal.pal_is_fru_prsnt(fru_id):  # Check if the fru is present
        sensor_ids_list = pal.pal_get_fru_sensor_list(fru_id)
        for sensor_id in sensor_ids_list:
            sensor_unit = sdr.sdr_get_sensor_units(fru_id, sensor_id)
            if sensor_unit in desired_sensor_units:
                try:
                    reading = pal.sensor_read(fru_id, sensor_id)
                    reading = round(reading, 2)
                    start_time = int(time.time()) - 60
                    sensor_history = pal.sensor_read_history(
                        fru_id, sensor_id, start_time
                    )
                except pal.LibPalError:
                    print(
                        "Failed to get reading for fru: {fru_name} , sensor_id: {sensor_id}".format(  # noqa: B950
                            fru_name=fru_name, sensor_id=sensor_id
                        )
                    )
                try:
                    sensor_thresh = sdr.sdr_get_sensor_thresh(fru_id, sensor_id)
                    sensor_name = sdr.sdr_get_sensor_name(fru_id, sensor_id)
                    sensor_details = SensorDetails(
                        sensor_name,
                        sensor_id,
                        fru_name,
                        reading,
                        sensor_thresh,
                        sensor_unit,
                        sensor_history,
                    )
                    sensor_details_list.append(sensor_details)
                except sdr.LibSdrError:
                    print(
                        "Failed to get sensor thresh for fru: {fru_name} , sensor_id: {sensor_id}".format(  # noqa: B950
                            fru_name=fru_name, sensor_id=sensor_id
                        )
                    )
    return sensor_details_list


def get_sensor_details_helper(
    sensor_units: t.List[str], fru_name: t.Optional[str] = None
) -> t.List[SensorDetails]:
    all_sensor_details = []
    if fru_name is None:
        fru_name_list = get_single_sled_frus()
        for fru in fru_name_list:
            all_sensor_details += get_sensor_details(fru, sensor_units)
    else:
        all_sensor_details = get_sensor_details(fru_name, sensor_units)

    return all_sensor_details


FruInfo = t.NamedTuple(
    "FruInfo",
    [("fru_name", str), ("manufacturer", str), ("serial_number", int), ("model", str)],
)


# as a workaround for lru_cache because its not compatible with async func
__CACHE_FRU_INFO = {}  # type: t.Dict[str, FruInfo]


async def get_fru_info_helper(fru_name: t.Optional[str] = None) -> t.List[FruInfo]:
    frus_info_list = []
    if fru_name is None:
        fru_name_list = get_single_sled_frus()
        for fru in fru_name_list:
            fru_info = await get_fru_info(fru)
            frus_info_list.append(fru_info)
    else:
        fru_info = await get_fru_info(fru_name)
        frus_info_list.append(fru_info)

    return frus_info_list


async def get_fru_info(fru_name: str) -> FruInfo:
    if fru_name in __CACHE_FRU_INFO:
        return __CACHE_FRU_INFO[fru_name]

    fru_details_dict = {}
    if is_fruid_util_available():  # then its a compute platform
        cmd = [FRUID_UTIL_PATH, fru_name, "--json"]
        _, fru_details, _ = await async_exec(cmd)
        fru_details_list = json.loads(fru_details)
        if fru_details_list:  # if its not empty
            fru_details_dict = fru_details_list[0]
    elif is_we_util_available():  # then its an fboss platform
        cmd = [WEUTIL_PATH]
        _, fru_details, _ = await async_exec(cmd)
        s_data = fru_details.split(os.linesep)
        for line in s_data:
            key_pair = line.split(":")
            if len(key_pair) > 1:
                fru_details_dict[key_pair[0]] = key_pair[1]
    else:
        raise NotImplementedError("Can't get fru info for fru : {}".format(fru_name))

    try:
        fru_info = FruInfo(
            fru_name,
            fru_details_dict["Product Manufacturer"].strip(),
            fru_details_dict["Product Serial"].strip(),
            fru_details_dict["Product Name"].strip(),
        )
        __CACHE_FRU_INFO[fru_name] = fru_info
        return fru_info
    except KeyError as e:
        server_logger.exception("key error ", exc_info=e)
        return FruInfo(
            fru_name=fru_name, manufacturer=None, serial_number=None, model=None
        )


def is_fruid_util_available() -> bool:
    return which(FRUID_UTIL_PATH) is not None


def is_we_util_available() -> bool:
    return which(WEUTIL_PATH) is not None


def get_single_sled_frus() -> t.List[str]:
    fru_name_map = pal.pal_fru_name_map()
    fru_list = []
    for fru_name in fru_name_map:
        if "slot" not in fru_name:
            fru_list.append(fru_name)
    return fru_list


def get_chassis_members_json() -> t.List[t.Dict[str, t.Any]]:
    if "slot4" in pal.pal_fru_name_map():
        # return chassis members for a multisled platform
        return [
            {"@odata.id": "/redfish/v1/Chassis/1"},
            {"@odata.id": "/redfish/v1/Chassis/server1"},
            {"@odata.id": "/redfish/v1/Chassis/server2"},
            {"@odata.id": "/redfish/v1/Chassis/server3"},
            {"@odata.id": "/redfish/v1/Chassis/server4"},
        ]
    else:
        # return chassis members for a single sled platform
        return [{"@odata.id": "/redfish/v1/Chassis/1"}]


def is_platform_supported() -> bool:
    """platforms that don't support pal and sdr functions for sensor related data"""
    UNSUPPORTED_PLATFORM_BUILDNAMES = ["yamp", "wedge", "wedge100"]
    return pal.pal_get_platform_name() not in UNSUPPORTED_PLATFORM_BUILDNAMES
