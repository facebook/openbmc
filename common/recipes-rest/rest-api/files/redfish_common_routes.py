import redfish_sensors
import rest_pal_legacy
from aiohttp import web
from aiohttp.web import Application
from redfish_account_service import get_account_service, get_accounts, get_roles
from redfish_bios_firmware_dumps import RedfishBIOSFirmwareDumps
from redfish_chassis import RedfishChassis
from redfish_computer_system import RedfishComputerSystems
from redfish_managers import (
    get_managers,
    get_managers_members,
    get_manager_ethernet,
    get_manager_network,
    get_ethernet_members,
    get_manager_log_services,
)
from redfish_powercycle import powercycle_post_handler, oobcycle_post_handler
from redfish_service_root import get_redfish, get_service_root, get_odata, get_metadata
from redfish_session_service import get_session_service, get_session


class Redfish:
    def __init__(self):
        self.computer_systems = RedfishComputerSystems()
        self.bios_firmware_dumps = RedfishBIOSFirmwareDumps()

    async def controller(self, request):
        return web.json_response()

    def setup_redfish_common_routes(self, app: Application):
        redfish_chassis = RedfishChassis()
        app.router.add_get("/redfish", get_redfish)
        app.router.add_get("/redfish/v1", get_service_root)
        app.router.add_get("/redfish/v1/", get_service_root)
        app.router.add_get("/redfish/v1/odata", get_odata)
        app.router.add_get("/redfish/v1/$metadata", get_metadata)
        app.router.add_get("/redfish/v1/AccountService", get_account_service)
        app.router.add_get("/redfish/v1/AccountService/Accounts", get_accounts)
        app.router.add_get("/redfish/v1/AccountService/Roles", get_roles)
        app.router.add_get("/redfish/v1/SessionService", get_session_service)
        app.router.add_get("/redfish/v1/SessionService/Sessions", get_session)
        app.router.add_get("/redfish/v1/Chassis", redfish_chassis.get_chassis)
        app.router.add_get("/redfish/v1/Chassis/1", redfish_chassis.get_chassis_members)
        app.router.add_get(
            "/redfish/v1/Chassis/1/Thermal",
            redfish_chassis.get_chassis_thermal,
        )
        app.router.add_get(
            "/redfish/v1/Chassis/1/Power", redfish_chassis.get_chassis_power
        )

        app.router.add_get(
            "/redfish/v1/Systems", self.computer_systems.get_collection_descriptor
        )
        app.router.add_get("/redfish/v1/Managers", get_managers)
        app.router.add_get("/redfish/v1/Managers/1", get_managers_members)
        app.router.add_get(
            "/redfish/v1/Managers/1/LogServices", get_manager_log_services
        )
        app.router.add_get(
            "/redfish/v1/Managers/1/EthernetInterfaces", get_manager_ethernet
        )
        app.router.add_get(
            "/redfish/v1/Managers/1/NetworkProtocol", get_manager_network
        )
        app.router.add_get(
            "/redfish/v1/Managers/1/EthernetInterfaces/1", get_ethernet_members
        )
        app.router.add_get(
            "/redfish/v1/Chassis/{fru_name}/Sensors",
            redfish_sensors.get_redfish_sensors_handler,
        )
        app.router.add_get(
            "/redfish/v1/Chassis/{fru_name}/Sensors/{sensor_id}",
            redfish_sensors.get_redfish_sensor_handler,
        )
        app.router.add_post(
            "/redfish/v1/Systems/{fru_name}/Actions/ComputerSystem.Reset",
            powercycle_post_handler,
        )
        app.router.add_post(
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
            oobcycle_post_handler,
        )

    def setup_multisled_routes(self, app: Application):
        no_of_slots = rest_pal_legacy.pal_get_num_slots()
        for i in range(1, no_of_slots + 1):  # +1 to iterate uptill last slot
            server_name = "server{}".format(i)
            redfish_chassis = RedfishChassis("slot{}".format(i))
            computer_system = self.computer_systems.get_server(server_name)
            bios_firmware_dumps = self.bios_firmware_dumps.get_server(server_name)
            app.router.add_get(
                "/redfish/v1/Chassis/{}".format(server_name),
                redfish_chassis.get_chassis_members,
            )
            app.router.add_get(
                "/redfish/v1/Chassis/{}/Power".format(server_name),
                redfish_chassis.get_chassis_power,
            )
            app.router.add_get(
                "/redfish/v1/Chassis/{}/Thermal".format(server_name),
                redfish_chassis.get_chassis_thermal,
            )
            app.router.add_post(
                "/redfish/v1/Chassis/{}/Thermal".format(server_name), self.controller
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}".format(server_name),
                computer_system.get_system_descriptor,
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}/Bios".format(server_name),
                computer_system.get_bios_descriptor,
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}/Bios/FirmwareInventory".format(server_name),
                computer_system.get_bios_firmware_inventory,
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}/Bios/FirmwareDumps".format(server_name),
                bios_firmware_dumps.get_collection_descriptor,
            )
            app.router.add_post(
                "/redfish/v1/Systems/{}".format(server_name)
                + "/Bios/FirmwareDumps/{DumpID}",
                bios_firmware_dumps.create_dump,
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}".format(server_name)
                + "/Bios/FirmwareDumps/{DumpID}",
                bios_firmware_dumps.get_dump_descriptor,
            )
            app.router.add_get(
                "/redfish/v1/Systems/{}".format(server_name)
                + "/Bios/FirmwareDumps/{DumpID}/Actions/BIOSFirmwareDump.ReadContent",
                bios_firmware_dumps.read_dump_content,
            )
            app.router.add_delete(
                "/redfish/v1/Systems/{}".format(server_name)
                + "/Bios/FirmwareDumps/{DumpID}",
                bios_firmware_dumps.delete_dump,
            )
