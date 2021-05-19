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
    # Add all attestation related imports here
    import obmc_attestation.measure
    import obmc_attestation.tpm2

    ATTESTATION_AVAILABLE = True
except ModuleNotFoundError:
    ATTESTATION_AVAILABLE = False
    # Doing this so that we don't break upstream
from node import node
from tree import tree


def get_tree_attestation() -> tree:
    tree_attestation = tree("attestation", node())
    if not ATTESTATION_AVAILABLE:
        # Return witout actually adding any endpoints/attaching
        # them to any logic
        return tree_attestation
    # GET /attestation/system_information
    # Always try to keep these in sync with the CLI
    tree_attestation.addChild(tree("system_information", NodeSystemInfo()))
    tpm_actions = ["ek-cert", "create-aik", "pcr-quote", "challenge-aik", "load-aik"]
    tree_attestation.addChild(tree("tpm", get_tpm_nodes(tpm_actions)))
    return tree_attestation


class NodeSystemInfo(node):
    @staticmethod
    # GET /attestation/system_information
    def getInformation(param={}):
        args = {}
        # Default args
        args["algo"] = "sha256"
        args["flash0"] = "/dev/flash0"
        args["flash1"] = "/dev/flash1"
        args["recal"] = False
        # We update the params if any were passed
        args.update(param)
        result = {}
        # Let's get the system hashes first
        result["system_hashes"] = obmc_attestation.measure.return_measure(args)
        return result


class NodeTPM(node):
    def __init__(self, actions=None):
        self.actions = actions
        self.tpm = obmc_attestation.tpm2.Tpm2v4()

    def doAction(self, data, param={}):
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
