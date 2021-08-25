import ctypes

"""This is a python wrapper for the aggregate-sensor.h
    to help you get aggregate sensors for a given platform"""

lib_aggregate = ctypes.CDLL("libaggregate-sensor.so.0.1")


class LibAggregateError(Exception):
    pass


def aggregate_sensor_name(index: int) -> str:
    c_sensor_name = ctypes.create_string_buffer(32)
    ret = lib_aggregate.aggregate_sensor_name(
        ctypes.c_uint8(index), ctypes.pointer(c_sensor_name)
    )
    if ret != 0:
        raise LibAggregateError("aggregate_sensor_name() returned " + str(ret))
    return c_sensor_name.value.decode("utf-8")


def aggregate_sensor_read(index: int) -> float:
    c_reading = ctypes.c_float(0)
    ret = lib_aggregate.aggregate_sensor_read(
        ctypes.c_uint8(index), ctypes.pointer(c_reading)
    )
    if ret != 0:
        raise LibAggregateError("aggregate_sensor_read() returned " + str(ret))
    return c_reading.value


def aggregate_sensor_init() -> None:
    ret = lib_aggregate.aggregate_sensor_init(None)
    if ret != 0:
        raise LibAggregateError("aggregate_sensor_init() returned " + str(ret))


def aggregate_sensor_count() -> int:
    c_count = ctypes.c_int(0)
    ret = lib_aggregate.aggregate_sensor_count(ctypes.pointer(c_count))
    if ret != 0:
        raise LibAggregateError("aggregate_sensor_count() returned " + str(ret))
    return c_count.value
