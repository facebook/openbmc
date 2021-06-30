from aiohttp import web
from aiohttp.web import Application


class Redfish:
    async def controller(self, request):
        return web.json_response()

    def setup_redfish_common_routes(self, app: Application):
        app.router.add_get("/redfish", self.controller)
        app.router.add_post("/redfish", self.controller)
        app.router.add_get("/redfish/v1", self.controller)
        app.router.add_post("/redfish/v1", self.controller)
        app.router.add_get("/redfish/v1/AccountService", self.controller)
        app.router.add_post("/redfish/v1/AccountService", self.controller)
        app.router.add_get("/redfish/v1/Chassis", self.controller)
        app.router.add_post("/redfish/v1/Chassis", self.controller)
        app.router.add_get("/redfish/v1/Chassis/1", self.controller)
        app.router.add_post("/redfish/v1/Chassis/1", self.controller)
        app.router.add_get("/redfish/v1/Chassis/1/Power", self.controller)
        app.router.add_post("/redfish/v1/Chassis/1/Power", self.controller)
        app.router.add_get("/redfish/v1/Chassis/1/Thermal", self.controller)
        app.router.add_post("/redfish/v1/Chassis/1/Thermal", self.controller)
        app.router.add_get("/redfish/v1/Managers", self.controller)
        app.router.add_post("/redfish/v1/Managers", self.controller)
        app.router.add_get("/redfish/v1/Managers/1", self.controller)
        app.router.add_post("/redfish/v1/Managers/1", self.controller)
        app.router.add_get("/redfish/v1/Managers/1/EthernetInterfaces", self.controller)
        app.router.add_post(
            "/redfish/v1/Managers/1/EthernetInterfaces", self.controller
        )
        app.router.add_get("/redfish/v1/Managers/1/NetworkProtocol", self.controller)
        app.router.add_post("/redfish/v1/Managers/1/NetworkProtocol", self.controller)
        app.router.add_get(
            "/redfish/v1/Managers/1/EthernetInterfaces/1", self.controller
        )
        app.router.add_post(
            "/redfish/v1/Managers/1/EthernetInterfaces/1", self.controller
        )
