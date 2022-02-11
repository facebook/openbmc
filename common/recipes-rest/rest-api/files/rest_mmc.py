import asyncio
import functools
import typing as t

import obmc_mmc
from aiohttp.log import server_logger


async def get_mmc_info() -> t.Dict[str, t.Dict[str, t.Union[int, str]]]:
    """
    Returns metrics for every mmc device found in the system in the
    format of {<dev_path>: {<metric_name>: <value>}}
    """
    loop = asyncio.get_event_loop()

    ret = {}
    for dev_path in _list_mmc_devices():
        ret[dev_path] = await loop.run_in_executor(None, _get_dev_info, dev_path)

    return ret


def _get_dev_info(dev_path: str) -> t.Dict[str, t.Union[int, str]]:
    """
    Returns health information of a given mmcblk device. Exceptions are suppressed and
    logged
    """
    ret = {}
    try:
        with obmc_mmc.mmc_dev(dev_path) as mmc:
            # Currently only exposes extcsd metrics, but can be extended later on
            extcsd = obmc_mmc.mmc_extcsd_read(mmc)
            cid = obmc_mmc.mmc_cid_read(mmc)

            lte = obmc_mmc.extcsd_life_time_estimate(extcsd)

            ret.update(
                {
                    # fields from cid
                    "CID_MID": cid.mid,
                    "CID_PNM": cid.pnm.decode(),
                    "CID_PRV_MAJOR": cid.prv_major,
                    "CID_PRV_MINOR": cid.prv_minor,
                    # extcsd fields
                    "EXT_CSD_PRE_EOL_INFO": lte.EXT_CSD_PRE_EOL_INFO,
                    "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A": lte.EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_A,  # noqa: B950
                    "EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B": lte.EXT_CSD_DEVICE_LIFE_TIME_EST_TYP_B,  # noqa: B950
                }
            )

    except obmc_mmc.LibObmcMmcException as e:
        server_logger.exception(
            "Error while attempting to fetch mmc health data for {dev_path}: {e}".format(  # noqa: B950
                dev_path=repr(dev_path), e=repr(e)
            )
        )

    return ret


# OPTIMISATION: Cache the list of mmc devices forever (assume /dev/mmcblk* devices are
# stable by the time the REST api is up)
@functools.lru_cache(maxsize=1)
def _list_mmc_devices() -> t.List[str]:
    return obmc_mmc.list_devices()
