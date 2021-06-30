import unittest

from aiohttp import web
from redfish_common_routes import Redfish


class TestCommonRoutes(unittest.TestCase):
    def test_common_routes(self):
        app = web.Application()
        redfish = Redfish()
        redfish.setup_redfish_common_routes(app)
        registered_routes = set()
        for route in app.router.resources():
            registered_routes.add(route.url())
        routes_expected = [
            "/redfish",
            "/redfish/v1",
            "/redfish/v1/AccountService",
            "/redfish/v1/Chassis",
            "/redfish/v1/Chassis/1",
            "/redfish/v1/Chassis/1/Power",
            "/redfish/v1/Chassis/1/Thermal",
            "/redfish/v1/Managers",
            "/redfish/v1/Managers/1",
            "/redfish/v1/Managers/1/EthernetInterfaces",
            "/redfish/v1/Managers/1/NetworkProtocol",
            "/redfish/v1/Managers/1/EthernetInterfaces/1",
        ]
        self.maxDiff = None
        self.assertEqual(sorted(routes_expected), sorted(registered_routes))
