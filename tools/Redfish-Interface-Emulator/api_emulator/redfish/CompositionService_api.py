# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# CompositionService API File

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
from .templates.CompositionService import get_CompositionService_instance

config = {}

INTERNAL_ERROR = 500


# EventService Singleton API
# EventService does not have a Singleton API


# CompositionService Collection API
class CompositionServiceAPI(Resource):

    def __init__(self, **kwargs):
        logging.info('CompositionServiceAPI init called')
        try:
            global config
            config = get_CompositionService_instance(kwargs)
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR

    # HTTP GET
    def get(self):
        logging.info('CompositionServiceAPI GET called')
        try:
            global config
            resp = config, 200
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    # HTTP PUT
    def put(self):
        logging.info('CompositionServiceAPI PUT called')
        return 'PUT is not a supported command for CompositionServiceAPI', 405

    # HTTP POST
    def patch(self):
        logging.info('CompositionServiceAPI POST called')
        return 'POST is not a supported command for CompositionServiceAPI', 405

    # HTTP PATCH
    def patch(self):
        logging.info('CompositionServiceAPI PATCH called')
        return 'PATCH is not a supported command for CompositionServiceAPI', 405

    # HTTP DELETE
    def delete(self):
        logging.info('CompositionServiceAPI DELETE called')
        return 'DELETE is not a supported command for CompositionServiceAPI', 405
