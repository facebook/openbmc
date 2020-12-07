_CACHE = {}

FPERSIST = 1


class KeyOperationFailure(Exception):
    pass


def kv_get(key, flags=None):
    if key not in _CACHE:
        raise KeyOperationFailure()
    return _CACHE[key]


def kv_set(key, value, flags=None):
    _CACHE[key] = value
