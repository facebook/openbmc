# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ResourceZone API File

"""
Collection API:  GET
Singleton  API:  GET
"""

import g

import sys, traceback
import logging
import copy
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Resource and SubResource imports
from .templates.ResourceZone import get_ResourceZone_instance

members = {}

INTERNAL_ERROR = 500


# ResourceZone Singleton API
class ResourceZoneAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('ResourceZoneAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('ResourceZoneAPI GET called')
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
        logging.info('ResourceZoneAPI PUT called')
        return 'PUT is not a supported command for ResourceZoneAPI', 405

    # HTTP POST
    def patch(self, ident):
        logging.info('ResourceZoneAPI POST called')
        return 'POST is not a supported command for ResourceZoneAPI', 405

    # HTTP PATCH
    def patch(self, ident):
        logging.info('ResourceZoneAPI PATCH called')
        return 'PATCH is not a supported command for ResourceZoneAPI', 405

    # HTTP DELETE
    def delete(self, ident):
        logging.info('ResourceZoneAPI DELETE called')
        return 'DELETE is not a supported command for ResourceZoneAPI', 405


# Resource Zone Collection API
class ResourceZoneCollectionAPI(Resource):

    def __init__(self):
        logging.info('ResourceZoneCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#ZoneCollection.ZoneCollection',
            '@odata.id': self.rb + 'CompositionService/ResourceZones',
            '@odata.type': '#ZoneCollection.ZoneCollection',
            'Name': 'Resource Zone Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('ResourceZoneCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('ResourceZoneCollectionAPI PUT called')
        return 'PUT is not a supported command for ResourceZoneCollectionAPI', 405

    # HTTP POST
    def patch(self):
        logging.info('ResourceZoneCollectionAPI POST called')
        return 'PATCH is not a supported command for ResourceZoneCollectionAPI', 405

    # HTTP PATCH
    def patch(self):
        logging.info('ResourceZoneCollectionAPI PATCH called')
        return 'PATCH is not a supported command for ResourceZoneCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('ResourceZoneCollectionAPI DELETE called')
        return 'DELETE is not a supported command for ResourceZoneCollectionAPI', 405


# CreateResourceZone
#
# Called internally to create instances of a subresource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateResourceZone(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateResourceZone init called')
        logging.debug(kwargs)
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])
            logging.debug(wildcards)

    def put(self,ident):
        logging.info('CreateResourceZone put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_ResourceZone_instance(wildcards)
            members[ident]=config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateResourceZone init exit')
        return resp

    def post(self,rb, ident,label,resource):
        logging.info('CreateResourceZone post called')
        try:
            global wildcards
            if ident in members:
                if label == "ResourceBlocks":
                    parameter = dict()
                    parameter["@odata.id"] = rb + "CompositionService/ResourceBlocks/" + resource
                    members[ident]["Links"]["ResourceBlocks"].append(parameter)
                else:
                    traceback.print_exc()
                    resp = INTERNAL_ERROR

            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateResourceZone post exit')
        return resp
