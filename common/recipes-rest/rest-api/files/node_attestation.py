#!/usr/bin/env python3
#
# Copyright 2018-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#
try:
    # Add all bmc attestation related imports here
    import obmc_attestation.helpers
    import obmc_attestation.measure
    import obmc_attestation.tpm2

    BMC_ATTESTATION_AVAILABLE = True
except ImportError:
    BMC_ATTESTATION_AVAILABLE = False
    # Doing this so that we don't break upstream

try:
    # Add all device attestation related imports here
    import device_attestation.measure

    DEVICE_ATTESTATION_AVAILABLE = True
except ImportError:
    DEVICE_ATTESTATION_AVAILABLE = False
    # Doing this so that we don't break obmc attestation

from aiohttp.web import Application
from compute_rest_shim import RestShim
from node import node

ACCEPTABLE_ALGORITHMS = ["sha1", "sha256"]


def setup_attestation_endpoints(app: Application) -> None:
    attestation_shim = RestShim(node(), "/api/attestation")
    app.router.add_get(attestation_shim.path, attestation_shim.get_handler)
    if not BMC_ATTESTATION_AVAILABLE:
        # Return witout actually adding any endpoints/attaching
        # them to any logic
        return
    # GET /attestation/system_information
    # Always try to keep these in sync with the CLI
    sysinfo_shim = RestShim(NodeSystemInfo(), "/api/attestation/system_information")
    app.router.add_get(sysinfo_shim.path, sysinfo_shim.get_handler)

    tpm_actions = ["ek-cert", "create-aik", "pcr-quote", "challenge-aik", "load-aik"]
    tpm_shim = RestShim(get_tpm_nodes(tpm_actions), "/api/attestation/tpm")
    app.router.add_get(tpm_shim.path, tpm_shim.get_handler)
    app.router.add_post(tpm_shim.path, tpm_shim.post_handler)

    if DEVICE_ATTESTATION_AVAILABLE:
        device_attestation_actions = ["measurement", "device-info"]
        device_attestation_shim = RestShim(
            NodeDeviceAttestation(device_attestation_actions), "/api/attestation/device"
        )
        app.router.add_post(
            device_attestation_shim.path, device_attestation_shim.post_handler
        )


class NodeSystemInfo(node):
    @staticmethod
    # GET /attestation/system_information
    async def getInformation(param):
        args = {}
        # Default args
        args["algo"] = "sha256"
        args["flash0"] = "/dev/flash0"
        args["flash1"] = "/dev/flash1"
        args["recal"] = False
        # We update the params if any were passed
        for argument in param.keys():
            if argument not in ["algo"]:
                raise Exception("You are allowed to specify only algo")
        args.update(param)
        if args["algo"] not in ACCEPTABLE_ALGORITHMS:
            raise Exception(
                "Only acceptable algos are: {}".format(str(ACCEPTABLE_ALGORITHMS))
            )
        result = {}
        # Let's get the system hashes first
        result["system_hashes"] = obmc_attestation.measure.return_measure(args)
        tpm_object = obmc_attestation.tpm2.Tpm2v4()
        # Let's get the TPM static info like TPM version
        result["tpm_info"] = tpm_object.get_tpm_static_information()
        # Let's get the system static info like kernel version
        result[
            "system_info"
        ] = await obmc_attestation.helpers.get_system_static_information()
        return result


class NodeTPM(node):
    def __init__(self, actions=None):
        self.actions = actions
        self.tpm = obmc_attestation.tpm2.Tpm2v4()

    async def doAction(self, data, param):
        # We will delete the action key from the data
        # and directly pass it onto the underlying function
        # as kwargs hoping that the function callee
        # obeys arguments structure
        action = data["action"]
        del data["action"]
        # TODO
        # Probably convert these into
        # getattr(tpm2, function name) later on?
        if action == "ek-cert":
            return self.tpm.get_ek_cert(**data)
        elif action == "create-aik":
            return self.tpm.tpm_create_aik(**data)
        elif action == "pcr-quote":
            return self.tpm.tpm_pcr_quote(**data)
        elif action == "challenge-aik":
            return self.tpm.tpm_challenge_aik(**data)
        elif action == "load-aik":
            return self.tpm.load_aik_into_tpm(**data)
        else:
            return {"status": "unimplented"}


def get_tpm_nodes(actions):
    return NodeTPM(actions)


class NodeDeviceAttestation(node):
    def __init__(self, actions=None):
        self.actions = actions

    async def doAction(self, data, param):
        # We will delete the action key from the data
        # and directly pass it onto the underlying function
        # as kwargs hoping that the function callee
        # obeys arguments structure
        action = data["action"]
        del data["action"]
        if action == "measurement":
            return device_attestation.measure.return_measure(**data)
        if action == "device-info":
            return await device_attestation.measure.return_device_info(**data)
        else:
            return {
                "status": "1",
                "message": "not supported",
            }
