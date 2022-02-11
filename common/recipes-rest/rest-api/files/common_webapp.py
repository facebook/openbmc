from aiohttp.web import Application
from common_middlewares import auth_enforcer, jsonerrorhandler, ratelimiter


class WebApp:
    _instance = None

    def __init__(self):
        raise RuntimeError("Call instance() instead")

    @classmethod
    def instance(cls) -> Application:
        if cls._instance is None:
            print("Creating new webapp")
            app = Application(
                middlewares=[jsonerrorhandler, ratelimiter, auth_enforcer]
            )
            cls._instance = app
        return cls._instance
