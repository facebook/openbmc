# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# ResourceBlock API File

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
from .templates.ResourceBlock import get_ResourceBlock_instance

members = {}

INTERNAL_ERROR = 500


# ResourceBlock Singleton API
class ResourceBlockAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('ResourceBlockAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('ResourceBlockAPI GET called')
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
        logging.info('ResourceBlockAPI PUT called')
        return 'PUT is not a supported command for ResourceBlockAPI', 405

    # HTTP POST
    def patch(self, ident):
        logging.info('ResourceBlockAPI POST called')
        return 'POST is not a supported command for ResourceBlockAPI', 405

    # HTTP PATCH
    def patch(self, ident):
        logging.info('ResourceBlockAPI PATCH called')
        return 'PATCH is not a supported command for ResourceBlockAPI', 405

    # HTTP DELETE
    def delete(self, ident):
        logging.info('ResourceBlockAPI DELETE called')
        return 'DELETE is not a supported command for ResourceBlockAPI', 405


# Resource Block Collection API
class ResourceBlockCollectionAPI(Resource):

    def __init__(self):
        logging.info('ResourceBlockCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#ResourceBlockCollection.ResourceBlockCollection',
            '@odata.id': self.rb + 'CompositionService/ResourceBlocks',
            '@odata.type': '#ResourceBlockCollection.ResourceBlockCollection',
            'Name': 'Resource Block Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('ResourceBlockCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('ResourceBlockCollectionAPI PUT called')
        return 'PUT is not a supported command for ResourceBlockCollectionAPI', 405

    # HTTP POST
    def patch(self):
        logging.info('ResourceBlockCollectionAPI POST called')
        return 'PATCH is not a supported command for ResourceBlockCollectionAPI', 405

    # HTTP PATCH
    def patch(self):
        logging.info('ResourceBlockCollectionAPI PATCH called')
        return 'PATCH is not a supported command for ResourceBlockCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('ResourceBlockCollectionAPI DELETE called')
        return 'DELETE is not a supported command for ResourceBlockCollectionAPI', 405


# CreateResourceBlock
#
# Called internally to create instances of a subresource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateResourceBlock(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateResourceBlock init called')
        logging.debug(kwargs)
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])
            logging.debug(wildcards)

    def put(self,ident):
        logging.info('CreateResourceBlock put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_ResourceBlock_instance(wildcards)
            members[ident]=config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateResourceBlock put exit')
        return resp

    def post(self,rb, ident,label,resource):
        logging.info('CreateResourceBlock post called')
        try:
            global wildcards
            if ident in members:

                if label == "linkSystem":
                    parameter = dict()
                    parameter["@odata.id"] = rb + "Systems/" + resource
                    members[ident]["Links"]["ComputerSystems"].append(parameter)
                elif label == "linkChassis":
                    parameter = dict()
                    parameter["@odata.id"] = rb + "Chassis/" + resource
                    members[ident]["Links"]["Chassis"].append(parameter)
                elif label == "linkZone":
                    parameter = dict()
                    parameter["@odata.id"] = rb + "CompositionService/ResourceZones/" + resource
                    members[ident]["Links"]["Zones"].append(parameter)
                else:
                    parameter = dict()
                    parameter["@odata.id"] = rb + "CompositionService/ResourceBlocks/" + ident +"/" + label + "/" + resource
                    members[ident][label].append(parameter)

            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateResourceBlock post exit')
        return resp
