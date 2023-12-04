#!/usr/bin/python3

import argparse
import ctypes
import datetime
import fcntl
import mmap
import os
import stat

from typing import List, NamedTuple, Optional

import pal

rtld = ctypes.cdll.LoadLibrary(None)
_shm_open = rtld.shm_open


class SensorStats(NamedTuple):
    """
    Named tuple containing basic stats of sensor values
    """

    minValue: float
    maxValue: float
    sumValue: float
    count: int
    time: Optional[datetime.datetime]

    def __str__(self):
        if self.count > 0:
            ret = "min=%.3f max=%.3f avg=%.3f count=%d" % (
                self.minValue,
                self.maxValue,
                self.sumValue / float(self.count),
                self.count,
            )
        else:
            ret = "min=NA max=NA avg=NA count=0"
        if self.time is not None:
            ret = "%s " % (self.time) + ret
        return ret


def auto_int(x: str) -> int:
    """
    Auto convert hex to int.
    """
    return int(x, 0)


def auto_date(x: str) -> datetime.datetime:
    """
    Convert date to datetime.
    """
    return datetime.datetime.strptime(x, "%Y-%m-%d:%H:%M:%S")


class SensorValue(ctypes.Structure):
    """
    Abstract SensorValue
    """

    def isRange(self, tf: datetime.datetime, tt: datetime.datetime) -> str:
        return False

    def getValue(self) -> SensorStats:
        """
        Returns a SensorStats version of the current value.
        """
        return SensorStats(0.0, 0.0, 0.0, 0, None)


class SensorFineValue(SensorValue):
    """
    finegrained-history sensor value. This is timestamped
    sensor value store. About 2k of these are stored before
    they get rolled up into coarsegrained-sensor-history.
    """

    _fields_ = [
        ("log_time", ctypes.c_long),
        ("value", ctypes.c_float),
    ]

    def isRange(self, ft: datetime.datetime, tt: datetime.datetime) -> bool:
        if self.log_time == 0:
            return False
        d = datetime.datetime.fromtimestamp(self.log_time)
        if ft is not None and d < ft:
            return False
        if d > tt:
            return False
        return True

    def getValue(self) -> SensorStats:
        return SensorStats(
            self.value,
            self.value,
            self.value,
            1,
            datetime.datetime.fromtimestamp(self.log_time),
        )


class SensorCoarseValue(SensorValue):
    """
    Coarse-grained history of sensor values. This is a time-stamped
    tuple of timestamp, min, max, avg, sum and sample count.
    (avg = sum / count). The inclusion of sum+count allows you to
    combine this with other SensorCourseValue to get a combined average.
    """

    _fields_ = [
        ("log_time", ctypes.c_long),
        ("sum", ctypes.c_float),
        ("count", ctypes.c_float),
        ("avg", ctypes.c_float),
        ("max", ctypes.c_float),
        ("min", ctypes.c_float),
    ]

    def isRange(self, ft: datetime.datetime, tt: datetime.datetime) -> bool:
        if self.log_time == 0:
            return False
        dt = datetime.datetime.fromtimestamp(self.log_time)
        df = dt - datetime.timedelta(hours=1)
        if ft is not None and ft > dt:
            return False
        if tt < df:
            return False
        return True

    def getValue(self) -> SensorStats:
        return SensorStats(
            self.min,
            self.max,
            self.sum,
            self.count,
            datetime.datetime.fromtimestamp(self.log_time),
        )


class SensorShare(ctypes.Structure):
    """
    Abstract class to work with sensor shared data structure.
    """

    @classmethod
    def key(cls, fru: str, snr: int):
        """
        Return the name of the named-shm where the circular buffer
        is stored.
        """
        return "%s_sensor%d" % (fru, snr)

    def get(self, ft: datetime.datetime, dt: datetime.datetime) -> List[SensorStats]:
        ret = []
        for i in range(0, self.count):
            idx = self.index - i - 1
            if idx < 0:
                idx += self.count
            if self.data[idx].isRange(ft, dt):
                ret.append(self.data[idx].getValue())
        return ret

    @classmethod
    def accumulate(cls, dstat: SensorStats, astat: SensorStats) -> SensorStats:
        """
        Helper function to accumulate dstat into astat and return the new
        version.
        """
        amin, amax, asum, acount, _ = astat
        asum += dstat.sumValue
        acount += dstat.count
        if dstat.maxValue > amax:
            amax = dstat.maxValue
        if dstat.minValue < amin:
            amin = dstat.minValue
        return SensorStats(amin, amax, asum, acount, None)

    def summary(
        self,
        from_time: datetime.datetime,
        to_time: datetime.datetime,
        astat: SensorStats,
    ) -> SensorStats:
        """
        Returns the summary in the required date range. Takes in starting
        accumulation point (which would allow us to combine this summary
        with another stat store)
        """
        for d in self.data:
            if d.isRange(from_time, to_time):
                astat = self.accumulate(d.getValue(), astat)
        return astat


class SensorFineShare(SensorShare):
    """
    fine-grained sensor history shared memory data structure
    """

    count = 2000
    _fields_ = [("index", ctypes.c_int), ("data", SensorFineValue * 2000)]


class SensorCoarseShare(SensorShare):
    """
    coarse-grained sensor history shared memory data structure
    """

    count = 24 * 30
    _fields_ = [
        ("index", ctypes.c_int),
        ("data", SensorCoarseValue * (24 * 30)),
    ]

    @classmethod
    def key(cls, fru, snr):
        return SensorShare.key(fru, snr) + "_coarse"


class SharedMemory:
    """
    Helper wrapper around safely reading the circular buffer
    in shared memory to get one of the sensor metric types.
    """

    def __init__(self, fru: str, snr: int, type: SensorShare):
        self.type = type
        self.name = self.type.key(fru, snr).encode()
        self.fd = -1
        self.share = None
        self.data = None

    def __enter__(self):
        self.fd = _shm_open(
            self.name,
            ctypes.c_int(os.O_RDWR | os.O_CREAT),
            ctypes.c_ushort(stat.S_IRUSR | stat.S_IWUSR),
        )
        if self.fd == -1:
            raise RuntimeError(os.strerror(ctypes.get_errno()))
        fcntl.flock(self.fd, fcntl.LOCK_EX)
        self.share = mmap.mmap(self.fd, ctypes.sizeof(self.type))
        self.data = self.type.from_buffer(self.share)
        return self

    def __exit__(self, type, value, traceback) -> None:
        self.data = None
        self.share.close()
        fcntl.flock(self.fd, fcntl.LOCK_UN)


def get_sensor_summary(
    fru: str, snr: int, from_time: datetime.datetime, to_time: datetime.datetime
) -> SensorStats:
    """
    Get the sensor summary
    """
    astat = SensorStats(float("inf"), float("-inf"), 0.0, 0, None)
    types = [SensorFineShare, SensorCoarseShare]
    for t in types:
        with SharedMemory(fru, snr, t) as sm:
            astat = sm.data.summary(from_time, to_time, astat)
    return astat


def get_sensor_history(
    fru: str, snr: int, from_time: datetime.datetime, to_time: datetime.datetime
) -> List[SensorStats]:
    types = [SensorFineShare, SensorCoarseShare]
    ret = []
    for t in types:
        with SharedMemory(fru, snr, t) as sm:
            ret.extend(sm.data.get(from_time, to_time))
    return ret


def print_summary(
    fru: str, snr: int, from_time: datetime.datetime, to_time: datetime.datetime
) -> None:
    """
    Print the summary of the provided sensor in the provided range.
    """
    astat = get_sensor_summary(fru, snr, from_time, to_time)
    print("0x%02x %s" % (snr, str(astat)))


def print_history(
    fru: str, snr: int, from_time: datetime.datetime, to_time: datetime.datetime
) -> None:
    """
    Print a trace (history) of all sensor changes in the provided change.
    """
    vals = get_sensor_history(fru, snr, from_time, to_time)
    for val in vals:
        print(str(val))


def main(
    fru,
    sensor_num,
    from_time: datetime.datetime,
    to_time: datetime.datetime,
    summary: bool,
) -> None:
    if not summary and sensor_num is None:
        print("To get full sensor value trace, please specify sensor_num")
        print("Printing history of sensor for all sensors can be confusing to parse")
        raise ValueError("Invalid Options")
    if sensor_num is None:
        fru_id = pal.pal_get_fru_id(fru)
        snr_list = pal.pal_get_fru_sensor_list(fru_id)
        for snr in snr_list:
            print_summary(fru, snr, from_time, to_time)
    else:
        if summary:
            print_summary(fru, sensor_num, from_time, to_time)
        else:
            print_history(fru, sensor_num, from_time, to_time)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sensor History Dumper")
    parser.add_argument(dest="fru", help="FRU Name")
    parser.add_argument(
        dest="sensor_num", nargs="?", type=auto_int, help="Sensor Num", default=None
    )
    parser.add_argument(
        "--from",
        dest="from_time",
        type=auto_date,
        help="From date, YYYY-MM-YY:HH:MM:SS Ex: 2023-11-10:13:10:01 (Default: Oldest)",
        default=None,
    )
    parser.add_argument(
        "--to",
        dest="to_time",
        type=auto_date,
        help="To date, YYYY-MM-YY:HH:MM:SS Ex: 2023-11-10:13:10:01 (Default: None)",
        default=datetime.datetime.now(),
    )
    parser.add_argument(
        "--summary",
        "-s",
        dest="summary",
        action="store_true",
        default=False,
        help="Print max/min/avg summary only",
    )
    args = parser.parse_args()
    main(args.fru, args.sensor_num, args.from_time, args.to_time, args.summary)
