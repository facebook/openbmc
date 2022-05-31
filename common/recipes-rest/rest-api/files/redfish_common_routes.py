import redfish_chassis
import redfish_sensors
from aiohttp import web
from aiohttp.web import Application
from redfish_account_service import get_account_service, get_accounts, get_roles
from redfish_bios_firmware_dumps import RedfishBIOSFirmwareDumps
from redfish_computer_system import RedfishComputerSystems
from redfish_fwinfo import (
    get_firmware_inventory_child,
    get_firmware_inventory_root,
    get_update_service_root,
)
from redfish_log_service import RedfishLogService
from redfish_managers import (
    get_ethernet_members,
    get_manager_ethernet,
    get_manager_log_services,
    get_manager_network,
    get_managers,
    get_managers_members,
)
from redfish_powercycle import oobcycle_post_handler, powercycle_post_handler
from redfish_service_root import get_metadata, get_odata, get_redfish, get_service_root
from redfish_session_service import get_session, get_session_service


class Redfish:
    def __init__(self):
        self.computer_systems = RedfishComputerSystems()
        self.bios_firmware_dumps = RedfishBIOSFirmwareDumps()
        self.log_service = RedfishLogService()

    async def controller(self, request):
        return web.json_response()

    def setup_redfish_common_routes(self, app: Application):
        redfish_log_service = self.log_service.get_log_service_controller()

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
        app.router.add_get(
            "/redfish/v1/Chassis/{fru_name}", redfish_chassis.get_chassis_member
        )
        app.router.add_patch(
            "/redfish/v1/Chassis/{fru_name}", redfish_chassis.set_power_state
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
        app.router.add_post(
            "/redfish/v1/Managers/1/Actions/Manager.Reset",
            oobcycle_post_handler,
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
        app.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices",
            redfish_log_service.get_log_services_root,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}",
            redfish_log_service.get_log_service,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}/Entries",
            redfish_log_service.get_log_service_entries,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{fru_name}/LogServices/{LogServiceID}"
            + "/Entries/{EntryID}",
            redfish_log_service.get_log_service_entry,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{server_name}",
            self.computer_systems.get_system_descriptor,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{server_name}/Bios",
            self.computer_systems.get_bios_descriptor,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareInventory",
            self.computer_systems.get_bios_firmware_inventory,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps",
            self.bios_firmware_dumps.get_collection_descriptor,
        )
        app.router.add_post(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/",
            self.bios_firmware_dumps.create_dump,
        )
        app.router.add_post(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}",
            self.bios_firmware_dumps.create_dump,
        )
        app.router.add_get(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}",
            self.bios_firmware_dumps.get_dump_descriptor,
        )
        app.router.add_get(
            (
                "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}/"
                "Actions/BIOSFirmwareDump.ReadContent"
            ),
            self.bios_firmware_dumps.read_dump_content,
        )
        app.router.add_delete(
            "/redfish/v1/Systems/{server_name}/Bios/FirmwareDumps/{DumpID}",
            self.bios_firmware_dumps.delete_dump,
        )
        app.router.add_get("/redfish/v1/UpdateService", get_update_service_root)
        app.router.add_get(
            "/redfish/v1/UpdateService/FirmwareInventory", get_firmware_inventory_root
        )
        app.router.add_get(
            "/redfish/v1/UpdateService/FirmwareInventory/{fw_name}",
            get_firmware_inventory_child,
        )
