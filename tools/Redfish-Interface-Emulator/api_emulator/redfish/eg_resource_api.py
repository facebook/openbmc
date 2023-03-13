# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EgResource API File
#
# Replace the strings "EgResource" and "EgSubResource" with
# the proper resource name and adjust the code as necessary for the
# HTTP commands.
#
# This API file goes in the api_emulator/redfish directory, and must
# be paired with an appropriate Resource template file in the
# api_emulator/redfish/templates directory. The resource_manager.py
# file in the api_emulator directory can then be edited to use these
# files to make the resource dynamic.

"""
These APIs are attached to Collection Resources or Singleton
Resources by the resource_manager.py file.

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
from .templates.eg_resource import get_EgResource_instance
from .eg_subresource_api import EgSubResourceCollectionAPI, EgSubResourceAPI, CreateEgSubResource

members = {}

done = 'false'
INTERNAL_ERROR = 500


# EgResource Singleton API
class EgResourceAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('EgResourceAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('EgResourceAPI GET called')
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
        logging.info('EgResourceAPI PUT called')
        return 'PUT is not a supported command for EgResourceAPI', 405

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    def post(self, ident):
        logging.info('EgResourceAPI POST called')
        try:
            global config
            config=get_EgResource_instance({'rb': g.rest_base, 'eg_id': ident})
            members.append(config)
            global done
            # Attach URIs for subordinate resources
            if  (done == 'false'):
                # Add APIs for subordinate resources
                collectionpath = g.rest_base + "EgResources/" + ident + "/EgSubResources"
                logging.info('collectionpath = ' + collectionpath)
                g.api.add_resource(EgSubResourceCollectionAPI, collectionpath, resource_class_kwargs={'path': collectionpath} )
                singletonpath = collectionpath + "/<string:ident>"
                logging.info('singletonpath = ' + singletonpath)
                g.api.add_resource(EgSubResourceAPI, singletonpath,  resource_class_kwargs={'rb': g.rest_base, 'eg_id': ident} )
                done = 'true'
            # Create an instance of subordinate resources
            #cfg = CreateSubordinateRes()
            #out = cfg.put(ident)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PATCH
    def patch(self, ident):
        logging.info('EgResourceAPI PATCH called')
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
        logging.info('EgResourceAPI DELETE called')
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


# EgResource Collection API
class EgResourceCollectionAPI(Resource):

    def __init__(self):
        logging.info('EgResourceCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#EgResourceCollection',
            '@odata.id': self.rb + 'EgResources',
            '@odata.type': '#EgResourceCollection.EgResourceCollection',
            'Name': 'EgResource Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('EgResourceCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('EgResourceCollectionAPI PUT called')
        return 'PUT is not a supported command for EgResourceCollectionAPI', 405

    def verify(self, config):
        # TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('EgResourceCollectionAPI POST called')
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
        logging.info('EgResourceCollectionAPI PATCH called')
        return 'PATCH is not a supported command for EgResourceCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('EgResourceCollectionAPI DELETE called')
        return 'DELETE is not a supported command for EgResourceCollectionAPI', 405


# CreateEgResource
#
# Called internally to create instances of a resource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateEgResource(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateEgResource init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Attach APIs for subordinate resource(s). Attach the APIs for
    # a resource collection and its singletons.
    def put(self,ident):
        logging.info('CreateEgResource PUT called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_EgResource_instance(wildcards)
            members.append(config)
            member_ids.append({'@odata.id': config['@odata.id']})
            # Attach subordinate resources
            collectionpath = g.rest_base + "EgResources/" + ident + "/EgSubResources"
            logging.info('collectionpath = ' + collectionpath)
            g.api.add_resource(EgSubResourceCollectionAPI, collectionpath, resource_class_kwargs={'path': collectionpath} )
            singletonpath = collectionpath + "/<string:ident>"
            g.api.add_resource(EgSubResourceAPI, singletonpath,  resource_class_kwargs={'rb': g.rest_base, 'eg_id': ident} )
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateEgResource PUT exit')
        return resp
