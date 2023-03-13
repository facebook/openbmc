# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# PCIe Switch API File
# TODO: Debug the code in this file

"""
Collection API:  GET, POST
Singleton  API:  GET, PUT, POST, PATCH, DELETE
"""

import g

import sys, traceback
import logging
import copy
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Resource and SubResource imports
from .templates.pcie_switch import get_PCIeSwitch_instance

members = {}

done = 'false'
INTERNAL_ERROR = 500


# PCIe Switch Singleton API
class PCIeSwitchAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('PCIeSwitchAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('PCIeSwitchAPI GET called')
        try:
            # Find the entry with the correct value for Id
            resp = 404
            if ident in members:
                resp = members[ident], 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    # TODO: Fix the PCIeSwitchAPI PUT command
    def put(self, ident):
        logging.info('PCIeSwitchAPI PUT called')
        return 'PUT is not a supported command for ManagerAPI', 405
    '''
    # Preserving the following in case it might help someone in the future
    # HTTP PUT
    # - On the first call, we add the API for PCIeSwitches, because sw_id is not available in 'init'.
    # - TODO: debug why this returns a 500 error, on the second put call
    def put(self,ident):
        print ('PCIeSwitchAPI put called')
        try:
            global config
            config=get_PCIeSwitch_instance(g.rest_base,ident)
            members.append(config)
            member_ids.append({'@odata.id': config['@odata.id']})
            global done
            print ('var = ' + done)
            g.api.add_resource(PCIePortsAPI, '/redfish/v1/PCIeSwitches/<string:sw_id>/Ports')
            g.api.add_resource(PCIePortAPI,  '/redfish/v1/PCIeSwitches/<string:sw_id>/Ports/<string:ident>')
            resp = config, 200
        except OSError:
            resp = make_response('Resources directory does not exist', 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        print ('PCIeSwitchAPI put exit')
        return resp
    '''

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    #
    # TODO: Fix the emulator-only PCIeSwitchAPI POST command
    def post(self, ident):
        logging.info('PCIeSwitchAPI POST called')
        return 'POST is not a supported command for PCIeSwitchAPI', 405

    # HTTP PATCH
    def patch(self, ident):
        logging.info('PCIeSwitchAPI PATCH called')
        raw_dict = request.get_json(force=True)
        try:
            # Update specific portions of the identified object
            for key, value in raw_dict.items():
                members[ident][key] = value
            resp = members[ident], 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP DELETE
    def delete(self, ident):
        logging.info('PCIeSwitchAPI DELETE called')
        try:
            # Find the entry with the correct value for Id
            resp = 404
            if ident in members:
                del(members[ident])
                resp = 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp


# PCIe Switch Collection API
class PCIeSwitchesAPI(Resource):

    def __init__(self):
        logging.info('PCIeSwitchesAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#PCIeSwitchCollection',
            '@odata.id': self.rb + 'PCIeSwitchCollection',
            '@odata.type': '#PCIeSwitchCollection.PCIeSwitchCollection',
            'Name': 'PCIe Switch Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('PCIeSwitchesAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('PCIeSwitchesAPI PUT called')
        return 'PUT is not a supported command for PCIeSwitchesAPI', 405

    def verify(self, config):
        #TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('PCIeSwitchesAPI POST called')
        try:
            config = request.get_json(force=True)
            ok, msg = self.verify(config)
            if ok:
                members[config['Id']] = config
                resp = config, 201
            else:
                resp = msg, 400
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PATCH
    def patch(self):
        logging.info('PCIeSwitchesAPI PATCH called')
        return 'PATCH is not a supported command for PCIeSwitchesAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('PCIeSwitchesAPI DELETE called')
        return 'DELETE is not a supported command for PCIeSwitchesAPI', 405


# CreatePCIeSwitch
# TODO: Fix CreatePCIeSwitch as part of the PCIeSwitchAPI POST command fix
#
# Called internally to create instances of a resource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreatePCIeSwitch(Resource):

    def __init__(self, **kwargs):
        logging.info('CreatePCIeSwitch init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Create instance
    def put(self, ident):
        logging.info('CreatePCIeSwitch put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config = get_PCIeSwitch_instance(wildcards)
            members[ident] = config

            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreatePCIeSwitchAPI init exit')
        return resp
