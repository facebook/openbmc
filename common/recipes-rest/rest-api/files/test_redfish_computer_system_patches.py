import unittest
import unittest.mock
from enum import Enum

import test_mock_modules  # noqa: F401


class FruCapability(Enum):
    FRU_CAPABILITY_SERVER = 5


def _pal_get_fru_capability(fruid: int):
    if fruid >= 1 and fruid <= 4:
        return [FruCapability.FRU_CAPABILITY_SERVER]
    return []


def patches():
    return [
        unittest.mock.patch(
            "rest_pal_legacy.pal_get_num_slots",
            create=True,
            return_value=4,
        ),
        unittest.mock.patch(
            "pal.pal_fru_name_map",
            create=True,
            return_value={"slot1": 1, "slot2": 2, "slot3": 3, "slot4": 4, "spb": 5},
        ),
        unittest.mock.patch(
            "pal.pal_get_platform_name",
            create=True,
            return_value="__unit-test__",
        ),
        unittest.mock.patch(
            "pal.FruCapability",
            create=True,
            new_callable=lambda: FruCapability,
        ),
        unittest.mock.patch(
            "pal.pal_get_fru_capability",
            create=True,
            new_callable=lambda: _pal_get_fru_capability,
        ),
        unittest.mock.patch(
            "aggregate_sensor.aggregate_sensor_init",
            create=True,
            return_value=None,
        ),
    ]
