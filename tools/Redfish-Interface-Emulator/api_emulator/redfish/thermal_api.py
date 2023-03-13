# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Thermal API File

"""
Collection API:  (None)
Singleton  API:  GET, PATCH
"""

import g

import sys, traceback
import logging
import copy
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Resource and SubResource imports
from .templates.thermal import get_thermal_instance

members = {}

INTERNAL_ERROR = 500


# Thermal API
class ThermalAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('ThermalAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('ThermalAPI GET called')
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
        logging.info('ThermalAPI PUT called')
        return 'PUT is not a supported command for ThermalAPI', 405

    # HTTP POST
    def post(self, ident):
        logging.info('ThermalAPI POST called')
        return 'POST is not a supported command for ThermalAPI', 405

    # HTTP PATCH
    def patch(self, ident):
        logging.info('ThermalAPI PATCH called')
        raw_dict = request.get_json(force=True)
        logging.info(raw_dict)
        try:
            # Update specific portions of the identified object
            logging.info(members[ident])
            for key, value in raw_dict.items():
                logging.info('Update ' + key + ' to ' + str(value))
                members[ident][key] = value
            logging.info(members[ident])
            resp = members[ident], 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP DELETE
    def delete(self, ident):
        logging.info('ThermalAPI DELETE called')
        return 'DELETE is not a supported command for ThermalAPI', 405


# ThermalCollection API
# Thermal does not have a collection API


# CreateEgSubResource
#
# Called internally to create instances of a subresource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateThermal(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateThermal init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])
            logging.debug(wildcards)#, wildcards.keys())

    # PUT
    # - Create the resource (since URI variables are avaiable)
    def put(self,ch_id):
        logging.info('CreateThermal put called')
        try:
            global wildcards
            config=get_thermal_instance(wildcards)
            members[ch_id]=config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp
