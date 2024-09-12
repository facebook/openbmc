# pyre

import traceback
from functools import wraps
from typing import Callable, Dict

from fixmybmc import status

_checks: Dict[str, "BmcCheck"] = {}


class BmcCheck:
    def __init__(
        self,
        name: str,
        func: Callable,
    ) -> None:
        self.name = name
        self.func = func
        self.result = None

    def run(self) -> None:
        try:
            result = self.func()
        except Exception as e:
            traceback.print_exc()
            result = status.Error(exception=e)

        if not isinstance(result, status.Status):
            te = TypeError(f"check function returned non-Status: {result!r}")
            result = status.Error(exception=te)

        self.result = result

    def __repr__(self) -> str:
        return f"<Check {self.name}>"


def bmcCheck(
    func=None,
):
    def check_decorator(func):
        name = func.__name__

        @wraps(func)
        def returnStatusOkWrapper(*args, **kwargs):
            result = func(*args, **kwargs)
            if result is None:
                return status.Ok()
            return result

        check = BmcCheck(
            name,
            func=returnStatusOkWrapper,
        )
        _checks[name] = check
        return check

    return check_decorator(func)


def load_checks() -> None:
    import fixmybmc.modules  # noqa: F401
