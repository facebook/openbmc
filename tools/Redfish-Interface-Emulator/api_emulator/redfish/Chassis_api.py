# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Chassis API File

"""
Collection API:  GET, POST
Singleton  API:  GET, POST, PATCH, DELETE
"""

import g

import sys, traceback
import logging
import copy
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Resource and SubResource imports
from .templates.Chassis import get_Chassis_instance
from .thermal_api import ThermalAPI, CreateThermal
from .power_api import PowerAPI, CreatePower

members = {}

INTERNAL_ERROR = 500


# Chassis Singleton API
class ChassisAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('ChassisAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('ChassisAPI GET called')
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
    def put(self, ident):
        logging.info('ChassisAPI PUT called')
        return 'PUT is not a supported command for ChassisAPI', 405

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    def post(self, ident):
        logging.info('ChassisAPI POST called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            wildcards['linkSystem'] = ['UpdateWithPATCH']
            wildcards['linkResourceBlocks'] = ['UpdateWithPATCH']
            wildcards['linkMgr'] = 'UpdateWithPATCH'
            config=get_Chassis_instance(wildcards)
            members[ident]=config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PATCH
    def patch(self, ident):
        logging.info('ChassisAPI PATCH called')
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
        logging.info('ChassisAPI DELETE called')
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


# Chassis Collection API
class ChassisCollectionAPI(Resource):

    def __init__(self):
        logging.info('ChassisCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#ChassisCollection.ChassisCollection',
            '@odata.id': self.rb + 'Chassis',
            '@odata.type': '#ChassisCollection.1.0.0.ChassisCollection',
            'Name': 'Chassis Collection',
            'Members@odata.count': len(members),
            'Members': [{'@odata.id': x['@odata.id']} for
                        x in list(members.values())]
        }

    # HTTP GET
    def get(self):
        logging.info('ChassisCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('ChassisCollectionAPI PUT called')
        return 'PUT is not a supported command for ChassisCollectionAPI', 405

    def verify(self, config):
        #TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('ChassisCollectionAPI POST called')
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
        logging.info('ChassisCollectionAPI PATCH called')
        return 'PATCH is not a supported command for ChassisCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('ChassisCollectionAPI DELETE called')
        return 'DELETE is not a supported command for ChassisCollectionAPI', 405


# CreateChassis
#
# Called internally to create instances of a resource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateChassis(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateChassis init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Create instance
    def put(self, ident):
        logging.info('CreateChassis put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config = get_Chassis_instance(wildcards)
            members[ident] = config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateChassis init exit')
        return resp
