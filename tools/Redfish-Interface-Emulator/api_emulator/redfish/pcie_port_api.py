# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# PCIe Port API File
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
from .templates.pcie_port import get_PCIePort_instance

members = {}

INTERNAL_ERROR = 500


# PCIe Port Singleton API
class PCIePortAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('PCIePortAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('PCIePortAPI GET called')
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
    # TODO: Fix the PCIePortAPI PUT command
    def put(self, ident):
        logging.info('PCIePortAPI PUT called')
        return 'PUT is not a supported command for ManagerAPI', 405
    '''
    # Preserving the following in case it might help someone in the future
    # HTTP PUT
    def put(self,sw_id,ident):
        try:
            global config
            config=get_PCIePort_instance(g.rest_base,sw_id,ident)
            members.append(config)
            member_ids.append({'@odata.id': config['@odata.id']} )
            resp = config, 200
        except OSError:
            resp = make_response('Resources directory does not exist', 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp
        '''

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    #
    # TODO: Fix the emulator-only PCIePortAPI POST command
    def patch(self, ident):
        logging.info('PCIePortAPI POST called')
        return 'POST is not a supported command for PCIePortAPI', 405

    # HTTP PATCH
    def patch(self, ident):
        logging.info('PCIePortAPI PATCH called')
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
        logging.info('PCIePortAPI DELETE called')
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


# PCIe Port Collection API
class PCIePortCollectionAPI(Resource):

    def __init__(self):
        logging.info('PCIePortCollectionAPI init called')
        self.rb = g.rest_base
        self.config = {
            '@odata.context': self.rb + '$metadata#PCIePortCollection',
            '@odata.id': self.rb + 'PCIePortCollection',
            '@odata.type': '#PCIePorts.1.0.0.PCIePortCollection',
            'Name': 'PCIe Port Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('PCIePortCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('PCIePortCollectionAPI PUT called')
        return 'PUT is not a supported command for PCIePortCollectionAPI', 405

    def verify(self, config):
        #TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('PCIePortCollectionAPI POST called')
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
        logging.info('PCIePortCollectionAPI PATCH called')
        return 'PATCH is not a supported command for PCIePortCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('PCIePortCollectionAPI DELETE called')
        return 'DELETE is not a supported command for PCIePortCollectionAPI', 405


# CreatePCIePort
# TODO: Fix CreatePCIePort as part of the PCIePortAPI POST command fix
#
# Called internally to create instances of a resource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreatePCIePort(Resource):

    def __init__(self, **kwargs):
        logging.info('CreatePCIePort init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Attach APIs for subordinate resource(s). Attach the APIs for a resource collection and its singletons
    def put(self,ident):
        logging.info('CreatePCIePort put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_PCIePort_instance(wildcards)
            members[ident]=config

            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreatePCIePortAPI init exit')
        return resp
