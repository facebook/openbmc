# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Singleton API: POST

import g
import requests
import os
import subprocess
import time

import sys, traceback
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource
from subprocess import check_output

#from .ComputerSystem_api import state_disabled, state_enabled

members={}
INTERNAL_ERROR = 500

class ResetAction_API(Resource):
    # kwargs is use to pass in the wildcards values to replace when the instance is created.
    def __init__(self, **kwargs):
        pass
    
    # HTTP POST
    def post(self,ident):
        from .ComputerSystem_api import state_disabled, state_enabled
        print ('ResetAction')
        state_disabled(ident)
        print ('State disabled')
        state_enabled(ident)
        print ('State enabled')
        return 'POST request completed', 200

    # HTTP GET
    def get(self,ident):
        print ('ResetAction')
        print (members)
        return 'GET is not supported', 405, {'Allow': 'POST'}

    # HTTP PATCH
    def patch(self,ident):
         return 'PATCH is not supported', 405, {'Allow': 'POST'}

    # HTTP PUT
    def put(self,ident):
         return 'PUT is not supported', 405, {'Allow': 'POST'}

    # HTTP DELETE
    def delete(self,ident):
         return 'DELETE is not supported', 405, {'Allow': 'POST'}
