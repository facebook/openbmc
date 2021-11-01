import unittest
from unittest import mock

from aiohttp import web


class TestCommonRoutes(unittest.TestCase):
    def setUp(self):
        super().setUp()

    def test_common_routes(self):
        app = web.Application()
        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(app)
        registered_routes = set()
        for route in app.router.resources():
            uri = route.get_info().get("path")
            if not uri:
                uri = route.get_info().get("formatter")
            registered_routes.add(uri)
        routes_expected = [
            "/redfish",
            "/redfish/v1",
            "/redfish/v1/",
            "/redfish/v1/odata",
            "/redfish/v1/$metadata",
            "/redfish/v1/AccountService",
            "/redfish/v1/AccountService/Accounts",
            "/redfish/v1/AccountService/Roles",
            "/redfish/v1/Chassis",
            "/redfish/v1/Chassis/1",
            "/redfish/v1/Chassis/{fru_name}/Sensors",
            "/redfish/v1/Chassis/{fru_name}/Sensors/{sensor_id}",
            "/redfish/v1/Chassis/1/Power",
            "/redfish/v1/Chassis/1/Thermal",
            "/redfish/v1/Managers",
            "/redfish/v1/Managers/1",
            "/redfish/v1/Managers/1/EthernetInterfaces",
            "/redfish/v1/Managers/1/NetworkProtocol",
            "/redfish/v1/Managers/1/EthernetInterfaces/1",
            "/redfish/v1/Managers/1/LogServices",
            "/redfish/v1/SessionService",
            "/redfish/v1/SessionService/Sessions",
            "/redfish/v1/Systems",
            "/redfish/v1/Systems/{fru_name}/Actions/ComputerSystem.Reset",
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
        ]
        self.maxDiff = None
        self.assertEqual(sorted(routes_expected), sorted(registered_routes))

    def test_multisled_routes(self):
        values = {
            "DumpID": "DumpID",
        }
        for pal_response in [0, 1, 4]:
            with self.subTest(pal_response=pal_response):
                pal_mock = unittest.mock.MagicMock()
                pal_mock.return_value = pal_response
                with mock.patch("rest_pal_legacy.pal_get_num_slots", pal_mock):
                    routes_expected = []
                    app = web.Application()
                    from redfish_common_routes import Redfish

                    redfish = Redfish()
                    redfish.setup_multisled_routes(app)
                for i in range(1, pal_response + 1):  # +1 to iterate uptill last slot
                    server_name = "server{}".format(i)
                    routes_expected.append("/redfish/v1/Chassis/{}".format(server_name))
                    routes_expected.append(
                        "/redfish/v1/Chassis/{}/Power".format(server_name)
                    )
                    routes_expected.append(
                        "/redfish/v1/Chassis/{}/Thermal".format(server_name)
                    )
                    routes_expected.extend(
                        [
                            "/redfish/v1/Systems/{}".format(server_name),
                            "/redfish/v1/Systems/{}/Bios".format(server_name),
                            "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(
                                server_name
                            ),
                            "/redfish/v1/Systems/{}/Bios/FirmwareDumps/DumpID".format(
                                server_name
                            ),
                            "/redfish/v1/Systems/{}/Bios/FirmwareDumps/DumpID/Actions/BIOSFirmwareDump.ReadContent".format(
                                server_name
                            ),
                            "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(
                                server_name
                            ),
                        ]
                    )
                registered_routes = set()
                for route in app.router.resources():
                    try:
                        registered_routes.add(str(route.url_for(**values)))
                    except TypeError:
                        registered_routes.add(str(route.url_for()))
                self.maxDiff = None
                self.assertEqual(sorted(routes_expected), sorted(registered_routes))
