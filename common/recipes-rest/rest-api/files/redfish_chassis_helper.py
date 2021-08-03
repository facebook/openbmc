import json
import os
import time
import typing as t
from shutil import which

import pal
import sdr
import sensors
from common_utils import async_exec


SensorDetails = t.NamedTuple(
    "SensorDetails",
    [
        ("sensor_name", str),
        ("sensor_number", int),
        ("fru_name", str),
        ("reading", float),
        # Not referencing sdr.ThreshSensor as sdr is unavailable on unit test env
        # only for platforms that support libpal
        ("sensor_thresh", t.Optional["sdr.ThreshSensor"]),
        ("sensor_unit", str),
        # Not referencing pal.SensorHistory as pal is unavailable on unit test env
        # only for platforms that support libpal
        ("sensor_history", t.Optional["pal.SensorHistory"]),
        ("ucr_thresh", t.Optional[float]),  # for older fboss platforms as an
        # alternative for sensors_thresh bc sensors.py only give us upper threshold
    ],
)

FRUID_UTIL_PATH = "/usr/local/bin/fruid-util"
WEUTIL_PATH = "/usr/bin/weutil"

""" sensor type constants as defined in sensors.py """
LIB_SENSOR_FAN = 1
LIB_SENSOR_TEMPERATURE = 2
LIB_SENSOR_POWER = 3

sensor_unit_dict = {
    LIB_SENSOR_FAN: "RPM",
    LIB_SENSOR_TEMPERATURE: "C",
    LIB_SENSOR_POWER: "Volts",
}
SAD_SENSOR = -99999  # default reading for values not found.


class SensorReadError(Exception):
    pass


def get_sensor_details_using_libpal(
    fru_name: str, desired_sensor_units: t.List[str]
) -> t.List[SensorDetails]:
    """Returns sensor details for platforms that support libpal.
    Those are compute and newer fboss platforms"""
    fru_name_map = pal.pal_fru_name_map()
    fru_id = fru_name_map[fru_name]
    sensor_details_list = []
    if not pal.pal_is_fru_prsnt(fru_id):  # Check if the fru is present
        raise NotImplementedError("Fru is not present {}".format(fru_name))

    sensor_ids_list = pal.pal_get_fru_sensor_list(fru_id)
    for sensor_id in sensor_ids_list:
        sensor_unit = sdr.sdr_get_sensor_units(fru_id, sensor_id)
        if sensor_unit in desired_sensor_units:
            try:
                reading = pal.sensor_read(fru_id, sensor_id)
                reading = round(reading, 2)
                start_time = int(time.time()) - 60
                sensor_history = pal.sensor_read_history(fru_id, sensor_id, start_time)
            except pal.LibPalError as e:
                raise SensorReadError(
                    "Failed to get reading for fru: {fru_name} , sensor_id: {sensor_id} : {e}".format(  # noqa: B950
                        fru_name=fru_name, sensor_id=sensor_id, e=repr(e)
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
                    None,
                )
                sensor_details_list.append(sensor_details)
            except sdr.LibSdrError as e:
                raise SensorReadError(
                    "Failed to get sensor thresh for fru: {fru_name} , sensor_id: {sensor_id} : {e}".format(  # noqa: B950
                        fru_name=fru_name, sensor_id=sensor_id, e=repr(e)
                    )
                )
    return sensor_details_list


def get_sensor_details_using_libpal_helper(
    sensor_units: t.List[str], fru_name: t.Optional[str] = None
) -> t.List[SensorDetails]:
    all_sensor_details = []
    if fru_name is None:
        fru_name_list = get_single_sled_frus()
        for fru in fru_name_list:
            all_sensor_details += get_sensor_details_using_libpal(fru, sensor_units)
    else:
        all_sensor_details = get_sensor_details_using_libpal(fru_name, sensor_units)

    return all_sensor_details


def get_older_fboss_sensor_details(
    fru_name: str, desired_sensor_type: int
) -> t.List[SensorDetails]:
    """Returns sensor details using sensors.py for older fboss
    platforms that don't support libpal i.e. yamp, wedge, wedge100"""
    sensor_details_list = []
    sensors.init()
    for chip in sensors.ChipIterator():
        for sensor in sensors.FeatureIterator(chip):
            if sensor.type == desired_sensor_type:
                chip_name = sensors.chip_snprintf_name(chip)
                sensor_tag = sensor.name.decode("utf-8")
                reading_key = sensor_tag + "_input"
                ucr_key = sensor_tag + "_max"
                sensor_name = chip_name + "." + sensors.get_label(chip, sensor)
                sfs = list(sensors.SubFeatureIterator(chip, sensor))
                sf_keys = []
                sf_vals = []
                subfeatures_dict = {}
                #  retrieving keys and values for subfeatures
                for sf in sfs:
                    sf_keys.append(sf.name.decode("utf-8"))
                    try:
                        sf_vals.append(sensors.get_value(chip, sf.number))
                    except sensors.ErrorAccessRead:
                        # in case of an error, we set default SAD_SENSOR
                        subfeatures_dict[reading_key] = SAD_SENSOR

                #  for sensors that don't have upper threshold as a feature
                if ucr_key not in sf_keys:
                    subfeatures_dict[ucr_key] = 0  # default upper threshold val

                for key, val in zip(sf_keys, sf_vals):
                    subfeatures_dict[key] = val

                sensor_details = SensorDetails(
                    sensor_name=sensor_name,
                    sensor_number=0,  # default bc sensors.py doesn't provide sensor id
                    fru_name=fru_name,
                    reading=subfeatures_dict[reading_key],
                    sensor_thresh=None,  # bc sensors.py doesn't provide ThreshSensor
                    sensor_unit=sensor_unit_dict[desired_sensor_type],
                    sensor_history=None,  # bc sensors.py doesn't provide sensor history
                    ucr_thresh=subfeatures_dict[ucr_key],
                )
                sensor_details_list.append(sensor_details)

    sensors.cleanup()
    return sensor_details_list


FruInfo = t.NamedTuple(
    "FruInfo",
    [("fru_name", str), ("manufacturer", str), ("serial_number", int), ("model", str)],
)


# as a workaround for lru_cache because its not compatible with async func
__CACHE_FRU_INFO = {}  # type: t.Dict[str, FruInfo]


async def get_fru_info_helper(fru_name: t.Optional[str] = None) -> t.List[FruInfo]:
    frus_info_list = []
    if is_libpal_supported():  # for compute and new fboss platforms
        if fru_name is None:  # for single sled platforms
            fru_name_list = get_single_sled_frus()
            for fru in fru_name_list:
                fru_info = await get_fru_info(fru)
                frus_info_list.append(fru_info)
        else:  # for multisled platforms
            fru_info = await get_fru_info(fru_name)
            frus_info_list.append(fru_info)
    else:  # for older fboss platforms
        fru_info = await get_fru_info("BMC")
        frus_info_list.append(fru_info)

    return frus_info_list


async def get_fru_info(fru_name: str) -> FruInfo:
    if fru_name in __CACHE_FRU_INFO:
        return __CACHE_FRU_INFO[fru_name]

    fru_details_dict = {}
    product_name_key = "Product Name"
    if is_fruid_util_available():  # then its a compute platform
        cmd = [FRUID_UTIL_PATH, fru_name, "--json"]
        _, fru_details, _ = await async_exec(cmd)
        fru_details_list = json.loads(fru_details)
        if fru_details_list:  # if its not empty
            fru_details_dict = fru_details_list[0]
        # update keys as per fruid util output
        product_manufacturer_key = "Product Manufacturer"
        product_serial_key = "Product Serial"
    elif is_we_util_available():  # then its an fboss platform
        cmd = [WEUTIL_PATH]
        _, fru_details, _ = await async_exec(cmd)
        s_data = fru_details.split(os.linesep)
        for line in s_data:
            key_pair = line.split(":")
            if len(key_pair) > 1:
                fru_details_dict[key_pair[0]] = key_pair[1]
        # update keys as per weutil output
        product_manufacturer_key = "System Manufacturer"
        product_serial_key = "Product Serial Number"
    else:
        raise NotImplementedError("Can't get fru info for fru : {}".format(fru_name))

    try:
        fru_info = FruInfo(
            fru_name,
            fru_details_dict[product_manufacturer_key].strip(),
            fru_details_dict[product_serial_key].strip(),
            fru_details_dict[product_name_key].strip(),
        )
        __CACHE_FRU_INFO[fru_name] = fru_info
        return fru_info
    except KeyError:
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
    if is_libpal_supported() and "slot4" in pal.pal_fru_name_map():
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


def is_libpal_supported() -> bool:
    """platforms that don't support libpal for sensor related data"""
    UNSUPPORTED_PLATFORM_BUILDNAMES = ["yamp", "wedge", "wedge100"]
    return pal.pal_get_platform_name() not in UNSUPPORTED_PLATFORM_BUILDNAMES
