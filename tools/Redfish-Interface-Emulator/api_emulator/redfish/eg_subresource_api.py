# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EgSubResource API File
#
# Replace the string "EgSubResource" with the proper resource
# name and adjust the code as necessary for the HTTP commands.
#
# This API file goes in the api_emulator/redfish directory, and must
# be paired with an appropriate SubResource template file in the
# api_emulator/redfish/templates directory. The resource_manager.py
# file in the api_emulator directory can then be edited to use these
# files to make the resource dynamic.

"""
These APIs are attached to subordinate Collection Resources or
subordinate Singleton Resources by the resource api files to which
they are subordinate.

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
from .templates.eg_subresource import get_EgSubResource_instance

members = {}

INTERNAL_ERROR = 500


# EgSubResource API
class EgSubResourceAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('EgSubResourceAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('EgSubResourceAPI GET called')
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
        logging.info('EgSubResourceAPI PUT called')
        return 'PUT is not a supported command for EgSubResourceAPI', 405

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    def post(self, ident):
        logging.info('EgSubResourceAPI POST called')
        try:
            global config
            global wildcards
            wildcards['id']= ident
            config=get_EgSubResource_instance(wildcards)
            members.append(config)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PATCH
    def patch(self, ident):
        logging.info('EgSubResourceAPI PATCH called')
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
        logging.info('EgSubResourceAPI DELETE called')
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


# EgSubResourceCollection API
class EgSubResourceCollectionAPI(Resource):

    def __init__(self):
        logging.info('EgSubResourceCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#EgSubResourceCollection.EgSubResourceCollection',
            '@odata.id': self.rb + 'EgSubResources',
            '@odata.type': '#EgSubResourceCollection.EgSubResourceCollection',
            'Name': 'EgSubResource Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('EgSubResourceCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    def verify(self, config):
        #TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP PUT
    def put(self):
        logging.info('EgSubResourceCollectionAPI PUT called')
        return 'PUT is not a supported command for EgSubResourceCollectionAPI', 405

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('EgSubResourceCollectionAPI POST called')
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
        logging.info('EgSubResourceCollectionAPI PATCH called')
        return 'PATCH is not a supported command for EgSubResourceCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('EgSubResourceCollectionAPI DELETE called')
        return 'DELETE is not a supported command for EgSubResourceCollectionAPI', 405


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
class CreateEgSubResource(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateEgSubResource init called')
        logging.debug(kwargs, kwargs.keys(), 'resource_class_kwargs' in kwargs)
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])
            logging.debug(wildcards, wildcards.keys())

    # Add subordinate resource
    def put(self,ident):
        logging.info('CreateEgSubResource PUT called')
        try:
            global config
            global wildcards
            config=get_EgSubResource_instance(wildcards)
            members.append(config)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateEgSubResource PUT exit')
        return resp
