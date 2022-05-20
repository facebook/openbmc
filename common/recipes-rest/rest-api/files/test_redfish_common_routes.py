import unittest

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
            "/redfish/v1/Chassis/{fru_name}",
            "/redfish/v1/Chassis/{fru_name}/Sensors",
            "/redfish/v1/Chassis/{fru_name}/Sensors/{sensor_id}",
            "/redfish/v1/Managers",
            "/redfish/v1/Managers/1",
            "/redfish/v1/Managers/1/EthernetInterfaces",
            "/redfish/v1/Managers/1/NetworkProtocol",
            "/redfish/v1/Managers/1/EthernetInterfaces/1",
            "/redfish/v1/Managers/1/LogServices",
            "/redfish/v1/SessionService",
            "/redfish/v1/SessionService/Sessions",
            "/redfish/v1/UpdateService",
            "/redfish/v1/UpdateService/FirmwareInventory",
            "/redfish/v1/UpdateService/FirmwareInventory/{fw_name}",
            "/redfish/v1/Systems",
            "/redfish/v1/Systems/{server_name}",
            "/redfish/v1/Systems/{server_name}/Bios",
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps",
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/",
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}",
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}/Actions/BIOSFirmwareDump.ReadContent",
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareInventory",
            "/redfish/v1/Systems/{fru_name}/Actions/ComputerSystem.Reset",
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}",
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}/Entries",
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}"
            + "/Entries/{EntryID}",
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
        ]
        self.maxDiff = None
        self.assertEqual(sorted(routes_expected), sorted(registered_routes))
