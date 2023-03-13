# Copyright Notice:
# Copyright 2016-2021 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# SessionService API File

"""
Collection API:  GET
Singleton  API:  (None)
"""

import g

import sys, traceback
import logging
import copy
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Resource and SubResource imports
from .templates.SessionService import get_SessionService_instance
from .sessions_api import SessionCollectionAPI, SessionAPI, CreateSession

config = {}

INTERNAL_ERROR = 500


# SessionService Singleton API
# SessionService does not have a Singleton API


# SessionService Collection API
class SessionServiceAPI(Resource):

    def __init__(self, **kwargs):
        logging.info('SessionServiceAPI init called')
        try:
            global config
            config = get_SessionService_instance(kwargs)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR

    # HTTP GET
    def get(self):
        logging.info('SessionServiceAPI GET called')
        try:
            global config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('SessionServiceAPI PUT called')
        return 'PUT is not a supported command for SessionServiceAPI', 405

    # HTTP POST
    def patch(self):
        logging.info('SessionServiceAPI POST called')
        return 'POST is not a supported command for SessionServiceAPI', 405

    # HTTP PATCH
    def patch(self):
        logging.info('SessionServiceAPI PATCH called')
        return 'PATCH is not a supported command for SessionServiceAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('SessionServiceAPI DELETE called')
        return 'DELETE is not a supported command for SessionServiceAPI', 405


# CreateSessionService
#
# Called internally to create instances of a resource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
#
# TODO: Determine need for CreateSessionService
class CreateSessionService(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateSessionService init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Attach APIs for subordinate resource(s). Attach the APIs for
    # a resource collection and its singletons.
    def put(self,ident):
        logging.info('CreateSessionService put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_SessionService_instance(wildcards)
            g.api.add_resource(SessionCollectionAPI,   '/redfish/v1/SessionService/Sessions')
            g.api.add_resource(SessionAPI,             '/redfish/v1/SessionService/Sessions/<string:ident>', resource_class_kwargs={'rb': g.rest_base})
            # Create an instance of subordinate subscription resource
            cfg = CreateSubscription()
            out = cfg.__init__(resource_class_kwargs={'rb': g.rest_base,'id':"1"})
            out = cfg.put("1")
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateSessionService put exit')
        return resp
