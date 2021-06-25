#!/usr/bin/env python


from aiohttp.web import Application
from compute_rest_shim import RestShim
from node_bmc import get_node_bmc
from node_fans import get_node_fans
from node_fcb import get_node_fcb
from node_flash import get_node_flash
from node_fruid import get_node_fruid
from node_health import get_node_health
from node_logs import get_node_logs
from node_pdpb import get_node_pdpb
from node_peb import get_node_peb
from node_sensors import get_node_sensors


# Initialize Platform specific Resource Tree
def setup_board_routes(app: Application, write_enabled: bool):

    # Add /api/fcb to represent fan control board
    fcb_shim = RestShim(get_node_fcb(), "/api/fcb")
    app.router.add_get(fcb_shim.path, fcb_shim.get_handler)
    # Add /api/pdpb
    pdpb_shim = RestShim(get_node_pdpb(), "/api/pdpb")
    app.router.add_get(pdpb_shim.path, pdpb_shim.get_handler)
    # Add /api/peb
    peb_shim = RestShim(get_node_peb(), "/api/peb")
    app.router.add_get(peb_shim.path, peb_shim.get_handler)

    # Add /api/fcb/fruid end point
    fcb_fruid_shim = RestShim(get_node_fruid("fcb"), "/api/fcb/fruid")
    app.router.add_get(fcb_fruid_shim.path, fcb_fruid_shim.get_handler)
    # Add /api/fcb/sensors end point
    fcb_sensors_shim = RestShim(get_node_sensors("fcb"), "/api/fcb/sensors")
    app.router.add_get(fcb_sensors_shim.path, fcb_sensors_shim.get_handler)
    app.router.add_post(fcb_sensors_shim.path, fcb_sensors_shim.post_handler)
    # Add /api/fcb/logs end point
    fcb_logs_shim = RestShim(get_node_logs("fcb"), "/api/fcb/logs")
    app.router.add_get(fcb_logs_shim.path, fcb_logs_shim.get_handler)
    app.router.add_post(fcb_logs_shim.path, fcb_logs_shim.post_handler)

    # Add /api/fcb/fans end point
    fcb_fans_shim = RestShim(get_node_fans(), "/api/fcb/fans")
    app.router.add_get(fcb_fans_shim.path, fcb_fans_shim.get_handler)

    # Add /api/pdpb/fruid end point
    pdpb_fruid_shim = RestShim(get_node_fruid("pdpb"), "/api/pdpb/fruid")
    app.router.add_get(pdpb_fruid_shim.path, pdpb_fruid_shim.get_handler)
    # Add /api/pdpb/sensors end point
    pdpb_sensors_shim = RestShim(get_node_sensors("pdpb"), "/api/pdpb/sensors")
    app.router.add_get(pdpb_sensors_shim.path, pdpb_sensors_shim.get_handler)
    app.router.add_post(pdpb_sensors_shim.path, pdpb_sensors_shim.post_handler)
    # Add /api/pdpb/logs end point
    pdpb_logs_shim = RestShim(get_node_logs("pdpb"), "/api/pdpb/logs")
    app.router.add_get(pdpb_logs_shim.path, pdpb_logs_shim.get_handler)
    app.router.add_post(pdpb_logs_shim.path, pdpb_logs_shim.post_handler)

    # Add /api/pdpb/flash end point
    pdpb_flash_shim = RestShim(get_node_flash(), "/api/pdpb/flash")
    app.router.add_get(pdpb_flash_shim.path, pdpb_flash_shim.get_handler)

    # Add /api/peb/fruid end point
    peb_fruid_shim = RestShim(get_node_fruid("peb"), "/api/peb/fruid")
    app.router.add_get(peb_fruid_shim.path, peb_fruid_shim.get_handler)
    # /api/peb/bmc end point
    peb_bmc_shim = RestShim(get_node_bmc(), "/api/peb/bmc")
    app.router.add_get(peb_bmc_shim.path, peb_bmc_shim.get_handler)
    app.router.add_post(peb_bmc_shim.path, peb_bmc_shim.post_handler)
    # /api/peb/sensors end point
    peb_sensors_shim = RestShim(get_node_sensors("peb"), "/api/peb/sensors")
    app.router.add_get(peb_sensors_shim.path, peb_sensors_shim.get_handler)
    app.router.add_post(peb_sensors_shim.path, peb_sensors_shim.post_handler)
    # Add /api/peb/health end point
    peb_health_shim = RestShim(get_node_health(), "/api/peb/health")
    app.router.add_get(peb_health_shim.path, peb_health_shim.get_handler)
    # /api/peb/logs end point
    peb_logs_shim = RestShim(get_node_logs("peb"), "/api/peb/logs")
    app.router.add_get(peb_logs_shim.path, peb_logs_shim.get_handler)
    app.router.add_post(peb_logs_shim.path, peb_logs_shim.post_handler)
