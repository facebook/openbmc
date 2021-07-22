import rest_pal_legacy
from aiohttp import web
from aiohttp.web import Application
from redfish_account_service import get_account_service
from redfish_chassis import RedfishChassis
from redfish_managers import (
    get_managers,
    get_managers_members,
    get_manager_ethernet,
    get_manager_network,
    get_ethernet_members,
)
from redfish_service_root import get_redfish, get_service_root


class Redfish:
    async def controller(self, request):
        return web.json_response()

    def setup_redfish_common_routes(self, app: Application):
        redfish_chassis = RedfishChassis()
        app.router.add_get("/redfish", get_redfish)
        app.router.add_post("/redfish", self.controller)
        app.router.add_get("/redfish/v1", get_service_root)
        app.router.add_post("/redfish/v1", self.controller)
        app.router.add_get("/redfish/v1/AccountService", get_account_service)
        app.router.add_post("/redfish/v1/AccountService", self.controller)
        app.router.add_get("/redfish/v1/Chassis", redfish_chassis.get_chassis)
        app.router.add_post("/redfish/v1/Chassis", self.controller)
        app.router.add_get("/redfish/v1/Chassis/1", redfish_chassis.get_chassis_members)
        app.router.add_post("/redfish/v1/Chassis/1", self.controller)
        app.router.add_get(
            "/redfish/v1/Chassis/1/Thermal",
            redfish_chassis.get_chassis_thermal,
        )
        app.router.add_post("/redfish/v1/Chassis/1/Thermal", self.controller)
        app.router.add_get(
            "/redfish/v1/Chassis/1/Power", redfish_chassis.get_chassis_power
        )
        app.router.add_post("/redfish/v1/Chassis/1/Power", self.controller)
        app.router.add_get("/redfish/v1/Managers", get_managers)
        app.router.add_post("/redfish/v1/Managers", self.controller)
        app.router.add_get("/redfish/v1/Managers/1", get_managers_members)
        app.router.add_post("/redfish/v1/Managers/1", self.controller)
        app.router.add_get(
            "/redfish/v1/Managers/1/EthernetInterfaces", get_manager_ethernet
        )
        app.router.add_post(
            "/redfish/v1/Managers/1/EthernetInterfaces", self.controller
        )
        app.router.add_get(
            "/redfish/v1/Managers/1/NetworkProtocol", get_manager_network
        )
        app.router.add_post("/redfish/v1/Managers/1/NetworkProtocol", self.controller)
        app.router.add_get(
            "/redfish/v1/Managers/1/EthernetInterfaces/1", get_ethernet_members
        )
        app.router.add_post(
            "/redfish/v1/Managers/1/EthernetInterfaces/1", self.controller
        )

    def setup_multisled_routes(self, app: Application):
        no_of_slots = rest_pal_legacy.pal_get_num_slots()
        for i in range(1, no_of_slots + 1):  # +1 to iterate uptill last slot
            server_name = "server{}".format(i)
            redfish_chassis = RedfishChassis("slot{}".format(i))
            app.router.add_get(
                "/redfish/v1/Chassis/{}".format(server_name),
                redfish_chassis.get_chassis_members,
            )
            app.router.add_post(
                "/redfish/v1/Chassis/{}".format(server_name), self.controller
            )
            app.router.add_get(
                "/redfish/v1/Chassis/{}/Power".format(server_name),
                redfish_chassis.get_chassis_power,
            )
            app.router.add_post(
                "/redfish/v1/Chassis/{}/Power".format(server_name), self.controller
            )
            app.router.add_get(
                "/redfish/v1/Chassis/{}/Thermal".format(server_name),
                redfish_chassis.get_chassis_thermal,
            )
            app.router.add_post(
                "/redfish/v1/Chassis/{}/Thermal".format(server_name), self.controller
            )
