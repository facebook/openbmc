# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# EventService API File

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
from .templates.EventService import get_EventService_instance
from .Subscriptions_api import SubscriptionCollectionAPI, SubscriptionAPI, CreateSubscription

config = {}

INTERNAL_ERROR = 500


# EventService Singleton API
# EventService does not have a Singleton API


# EventService Collection API
class EventServiceAPI(Resource):

    def __init__(self, **kwargs):
        logging.info('EventServiceAPI init called')
        try:
            global config
            config = get_EventService_instance(kwargs)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR

    # HTTP GET
    def get(self):
        logging.info('EventServiceAPI GET called')
        try:
            global config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('EventServiceAPI PUT called')
        return 'PUT is not a supported command for EventServiceAPI', 405

    # HTTP POST
    def patch(self):
        logging.info('EventServiceAPI POST called')
        return 'POST is not a supported command for EventServiceAPI', 405

    # HTTP PATCH
    def patch(self):
        logging.info('EventServiceAPI PATCH called')
        return 'PATCH is not a supported command for EventServiceAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('EventServiceAPI DELETE called')
        return 'DELETE is not a supported command for EventServiceAPI', 405


# CreateEventService
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
# TODO: Determine need for CreateEventService
class CreateEventService(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateEventService init called')
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])

    # Attach APIs for subordinate resource(s). Attach the APIs for
    # a resource collection and its singletons.
    def put(self,ident):
        logging.info('CreateEventService put called')
        try:
            global config
            global wildcards
            wildcards['id'] = ident
            config=get_EventService_instance(wildcards)
            g.api.add_resource(SubscriptionCollectionAPI,   '/redfish/v1/EventService/Subscriptions')
            g.api.add_resource(SubscriptionAPI,             '/redfish/v1/EventService/Subscriptions/<string:ident>', resource_class_kwargs={'rb': g.rest_base})
            # Create an instance of subordinate subscription resource
            cfg = CreateSubscription()
            out = cfg.__init__(resource_class_kwargs={'rb': g.rest_base,'id':"1"})
            out = cfg.put("1")
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateEventService put exit')
        return resp
