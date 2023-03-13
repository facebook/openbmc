# Copyright Notice:
# Copyright 2017-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Subscriptions API File

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
from .templates.Subscription import get_Subscription_instance

members = {}

INTERNAL_ERROR = 500


# Subscription Singleton API
class SubscriptionAPI(Resource):

    # kwargs is used to pass in the wildcards values to be replaced
    # when an instance is created via get_<resource>_instance().
    #
    # The call to attach the API establishes the contents of kwargs.
    # All subsequent HTTP calls go through __init__.
    #
    # __init__ stores kwargs in wildcards, which is used to pass
    # values to the get_<resource>_instance() call.
    def __init__(self, **kwargs):
        logging.info('SubscriptionAPI init called')
        try:
            global wildcards
            wildcards = kwargs
        except Exception:
            traceback.print_exc()

    # HTTP GET
    def get(self, ident):
        logging.info('SubscriptionAPI GET called')
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
        logging.info('SubscriptionAPI PUT called')
        return 'PUT is not a supported command for SubscriptionAPI', 405

    # HTTP POST
    # This is an emulator-only POST command that creates new resource
    # instances from a predefined template. The new instance is given
    # the identifier "ident", which is taken from the end of the URL.
    # PATCH commands can then be used to update the new instance.
    def post(self, ident):
        logging.info('SubscriptionAPI POST called')
        try:
            global config
            global wildcards
            wildcards['id']= ident
            config = get_Subscription_instance(wildcards)
            members.append(config)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('SubscriptionAPI POST exit')
        return resp

    # HTTP PATCH
    def patch(self, ident):
        logging.info('SubscriptionAPI PATCH called')
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
        logging.info('SubscriptionAPI DELETE called')
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


# Subscription Collection API
class SubscriptionCollectionAPI(Resource):

    def __init__(self):
        logging.info('SubscriptionCollectionAPI init called')
        self.rb = os.path.join (g.rest_base, 'EventService/')
        self.config = {
            '@odata.id': self.rb + 'Subscriptions',
            '@odata.type': '#EventDestinationCollection.EventDestinationCollection',
            'Name': 'Event Destination Collection',
            'Links': {}
        }
        self.config['Links']['Members@odata.count'] = len(members)
        self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for
                x in list(members.values())]

    # HTTP GET
    def get(self):
        logging.info('SubscriptionCollectionAPI GET called')
        try:
            resp = self.config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('SubscriptionCollectionAPI PUT called')
        return 'PUT is not a supported command for SubscriptionCollectionAPI', 405

    def verify(self, config):
        # TODO: Implement a method to verify that the POST body is valid
        return True,{}

    # HTTP POST
    # POST should allow adding multiple instances to a collection.
    # For now, this only adds one instance.
    # TODO: 'id' should be obtained from the request data.
    def post(self):
        logging.info('SubscriptionCollectionAPI POST called')
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
        logging.info('SubscriptionCollectionAPI PATCH called')
        return 'PATCH is not a supported command for SubscriptionCollectionAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('SubscriptionCollectionAPI DELETE called')
        return 'DELETE is not a supported command for SubscriptionCollectionAPI', 405


# CreateSubscription
#
# Called internally to create instances of a subresource. If the
# resource has subordinate resources, those subordinate resource(s)
# are created automatically.
#
# Note: In 'init', the first time through, kwargs may not have any
# values, so we need to check. The call to 'init' stores the path
# wildcards. The wildcards are used to modify the resource template
# when subsequent calls are made to instantiate resources.
class CreateSubscription(Resource):

    def __init__(self, **kwargs):
        logging.info('CreateSubscription init called')
        logging.debug(kwargs)
        if 'resource_class_kwargs' in kwargs:
            global wildcards
            wildcards = copy.deepcopy(kwargs['resource_class_kwargs'])
            logging.debug(wildcards)

    # Add subordinate resource
    def put(self,ident):
        logging.info('CreateSubscription put called')
        try:
            global config
            global wildcards
            config=get_Subscription_instance(wildcards)
            members.append(config)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        logging.info('CreateSubscription init exit')
        return resp
