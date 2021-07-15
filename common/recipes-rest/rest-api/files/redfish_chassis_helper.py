import typing as t

import pal
import sdr


TemperatureSensor = t.NamedTuple(
    "TemperatureSensor",
    [
        ("sensor_name", str),
        ("sensor_number", int),
        ("fru_name", str),
        ("reading_celsius", str),
        #  Not referencing sdr.ThreshSensor as sdr is unavailable on unit test env
        ("sensor_thresh", "sdr.ThreshSensor"),
    ],
)


def get_temperature_sensors(fru_name: str) -> t.List[TemperatureSensor]:
    fru_name_map = pal.pal_fru_name_map()
    fru_id = fru_name_map[fru_name]
    temperature_sensors = []
    if pal.pal_is_fru_prsnt(fru_id):  # Check if the fru is present
        sensor_ids_list = pal.pal_get_fru_sensor_list(fru_id)
        for sensor_id in sensor_ids_list:
            if "C" in sdr.sdr_get_sensor_units(fru_id, sensor_id):
                try:
                    reading = pal.sensor_read(fru_id, sensor_id)
                    reading = round(reading, 2)
                except pal.LibSdrError:
                    print(
                        "Failed to get reading for fru: {fru_name} , sensor_id: {sensor_id}".format(  # noqa: B950
                            fru_name=fru_name, sensor_id=sensor_id
                        )
                    )
                try:
                    sensor_thresh = sdr.sdr_get_sensor_thresh(fru_id, sensor_id)
                    sensor_name = sdr.sdr_get_sensor_name(fru_id, sensor_id)
                    temperature_sensor = TemperatureSensor(
                        sensor_name, sensor_id, fru_name, reading, sensor_thresh
                    )
                    temperature_sensors.append(temperature_sensor)
                except sdr.LibSdrError:
                    print(
                        "Failed to get sensor thresh for fru: {fru_name} , sensor_id: {sensor_id}".format(  # noqa: B950
                            fru_name=fru_name, sensor_id=sensor_id
                        )
                    )
    return temperature_sensors


def get_temperature_sensors_helper(fru_name=None) -> t.List[TemperatureSensor]:
    temperature_sensors = []
    if fru_name is None:
        fru_name_list = get_single_sled_frus()
        for fru in fru_name_list:
            temperature_sensors += get_temperature_sensors(fru)
    else:
        temperature_sensors = get_temperature_sensors(fru_name)

    return temperature_sensors


def get_single_sled_frus() -> t.List[str]:
    fru_name_map = pal.pal_fru_name_map()
    fru_list = []
    for fru_name in fru_name_map:
        if "slot" not in fru_name:
            fru_list.append(fru_name)
    return fru_list


def is_platform_supported() -> bool:
    UNSUPPORTED_PLATFORMS = ["yamp", "wedge40", "wedge100"]
    return pal.pal_get_platform_name() not in UNSUPPORTED_PLATFORMS
