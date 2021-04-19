import ctypes
import glob
import os
import typing as t

clibobmc_mmc = ctypes.CDLL("libobmc-mmc.so.0.1")

MMC_EXTCSD_SIZE = 512  # from mmc_int.h


class LibObmcMmcException(Exception):
    pass


## High level structs
ExtCsdLifeTimeEstimate = t.NamedTuple(
    # Fields from include/linux/mmc/mmc.h
    "ExtCsdLifeTimeEstimate",
    [
        ("EXT_CSD_PRE_EOL_INFO", int),
        ("EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A", int),
        ("EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B", int),
    ],
)


class mmc_dev:
    # Convenience context manager to ensure we don't forget to close
    # the mmc device after we're done with them
    def __init__(self, dev_path: str):
        self._mmc = mmc_dev_open(dev_path)

    def __enter__(self):
        return self._mmc

    def __exit__(self, *args):
        mmc_dev_close(self._mmc)


## Wrapped structs
class mmc_dev_t(ctypes.Structure):
    # Opaque type, just defining a struct as it's much better than passing
    # void pointers around
    pass


class mmc_extcsd_t(ctypes.Structure):
    _fields_ = [("raw", ctypes.c_uint8 * MMC_EXTCSD_SIZE)]


class mmc_cid_t(ctypes.Structure):
    """
    From mmc_int.h
    Structure to hold eMMC CID. Refer to JDDEC Standard, section 7.2 for
    details.
    """

    _fields_ = [
        ("mid", ctypes.c_uint8),  # Manufacturer ID
        ("cbx", ctypes.c_uint8),  # Device/BGA
        ("oid", ctypes.c_uint8),  # OEM/Application ID
        ("pnm", ctypes.c_char * 7),  # Product Name
        ("prv_major", ctypes.c_uint8),  # Product revision (major)
        ("prv_minor", ctypes.c_uint8),  # Product revision (minor)
        ("psn", ctypes.c_uint32),  # Product serial number
        ("mdt_month", ctypes.c_uint8),  # Manufacturing date (month)
        ("mdt_year", ctypes.c_uint8),  # Manufacturing date (month)
    ]


## Wrapped functions, see mmc_int.h for docs
def mmc_dev_open(dev_path: str) -> mmc_dev_t:
    if not os.path.exists(dev_path):
        raise LibObmcMmcException(
            "Provided mmc device path '{dev_path}' does not exist".format(
                dev_path=dev_path
            )
        )

    ret = clibobmc_mmc.mmc_dev_open(dev_path.encode("utf-8"))
    if ret == 0:
        raise LibObmcMmcException("mmc_dev_open() returned NULL")

    return mmc_dev_t.from_address(ret)


def mmc_dev_close(mmc: mmc_dev_t) -> None:
    clibobmc_mmc.mmc_dev_close(ctypes.pointer(mmc))


def mmc_cid_read(mmc: mmc_dev_t) -> mmc_cid_t:
    cid = mmc_cid_t()

    ret = clibobmc_mmc.mmc_cid_read(ctypes.pointer(mmc), ctypes.pointer(cid))
    if ret != 0:
        raise LibObmcMmcException("mmc_cid_read() returned " + str(ret))

    return cid


def mmc_extcsd_read(mmc: mmc_dev_t) -> mmc_extcsd_t:
    extcsd = mmc_extcsd_t()

    ret = clibobmc_mmc.mmc_extcsd_read(ctypes.pointer(mmc), ctypes.pointer(extcsd))
    if ret != 0:
        raise LibObmcMmcException("mmc_extcsd_read() returned " + str(ret))

    return extcsd


## Utils
def list_devices() -> t.List[str]:
    return glob.glob("/dev/mmcblk[0-9]")


def extcsd_life_time_estimate(extcsd: mmc_extcsd_t) -> ExtCsdLifeTimeEstimate:
    "Convenience function to extract life time estimate from extcsd"
    # offsets from mmc.h
    return ExtCsdLifeTimeEstimate(
        EXT_CSD_PRE_EOL_INFO=extcsd.raw[267],
        EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A=extcsd.raw[268],
        EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B=extcsd.raw[269],
    )
