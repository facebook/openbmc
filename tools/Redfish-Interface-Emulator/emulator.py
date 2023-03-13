#!/usr/bin/env python2
#
# Copyright Notice:
# Copyright 2016-2021 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Redfish Interface Emulator - main
#   python emulator.py

import os
import json
import argparse
import traceback
import xml.etree.ElementTree as ET
import logging
import copy

logging.basicConfig(level=logging.DEBUG)

import g

# Flask Imports
from flask import Flask, request, make_response, render_template
from flask_restful import reqparse, Api, Resource

# Emulator Imports
from api_emulator.version import __version__
from api_emulator.resource_manager import ResourceManager
from api_emulator.static_resource_manager import StaticResourceManager
from api_emulator.exceptions import CreatePooledNodeError, ConfigurationError, RemovePooledNodeError
from api_emulator.resource_dictionary import ResourceDictionary

from infragen.populate import populate

# Trays to load into the resource manager
TRAYS = None
SPEC = None
MODE = None
MOCKUPFOLDERS = None
STATIC = None

CONFIG = 'emulator-config.json'

# Base URL of the RESTful interface
REST_BASE = '/redfish/v1/'
g.rest_base = REST_BASE

# Creating the ResourceManager
resource_manager = None
resource_dictionary = None

# Parse REST request for Action
parser = reqparse.RequestParser()
parser.add_argument('Action', type=str, required=True)

# Read emulator-config file.
# If running on Cloud, use dyanaically assigned port
with open(CONFIG, 'r') as f:
    config = json.load(f)
    try:
        MODE = config['MODE']
    except:
        pass

    try:
        POPULATE = config['POPULATE']
    except:
        pass

    try:
        STATIC = config['STATIC']
    except:
        pass

if(MODE=='Cloud'):
    port = int(os.getenv("PORT"))

# Execution starts a main(), at end of file

def init_resource_manager():
    """
    Initializes the resource manager
    """
    global resource_manager
    global REST_BASE
    global TRAYS
    global SPEC

    if (STATIC=='Enable'):
        print (' * Using static mockup')
        resource_manager = StaticResourceManager(REST_BASE, SPEC,MODE,TRAYS)
    else:
        print (' * Using dynamic emulation')
        resource_manager = ResourceManager(REST_BASE, SPEC,MODE,TRAYS)


    # If POPULATE is specified in emulator-config.json, INFRAGEN is called to populate emulator (i.e. with Chassi, CS, Resource Blocks, etc) according to specified file
    try:
        POPULATE
    except:
        pass
    else:
        if os.path.exists(POPULATE):
            with open(POPULATE, 'r') as f:
                infragen_config = json.load(f)
            populate(infragen_config.get('POPULATE',10))

    resource_dictionary = ResourceDictionary()


def error_response(msg, status, jsonify=False):
    data = {
        'Status': status,
        'Message': '{}'.format(msg)
    }
    if jsonify:
        data = json.dumps(data, indent=4)
    return data, status


INTERNAL_ERROR = error_response('Internal Server Error', 500)


class PathError(Exception):
    pass

@g.api.representation('application/xml')
def output_xml(data, code, headers=None):
    resp = make_response(data, code)
    resp.headers.extend(headers or {})
    resp.headers['Content-Type'] = 'text/xml; charset=ISO-8859-1'
    return resp

@g.api.representation('application/json')
def output_json(data, code, headers=None):
    """
    Overriding how JSON is returned by the server so that it looks nice
    """
    resp = make_response(json.dumps(data, indent=4), code)
    resp.headers.extend(headers or {})
    return resp



# The following code provides a mechanism for the Redfish client to either
#    - Emulator Service Root
#    - Control the emulator
#    - Test code fragments
#
# To control the emulator:
#    - Issuing a DELETE /redfish/v1/reset to reset the emulator
#
# To test code fragments
#    - Issuing a POST with an Action to /redfish/v1/Chassis/{id} or /redfish/v1/Systems/{id} to perform action.
#       - Assumes {id} is an integer.
#       - Action may be ApplySettings, Reset, Subscribe
#    - Issuing a GET
#    - Issuing a DELETE /redfish/v1/xxx/{id} to remove a pooled node (need to add checks)
#
class RedfishAPI(Resource):
    def __init__(self):
        # Dictionary of actions and their method
        self.actions = {
            'CreateGenericComputerSystem': self.create_system,
            'ApplySettings':self.update_system,
            'Reset': self.create_system,
            'Subscribe': self.subscribe_events }

        if resource_manager.spec == 'Redfish':
            self.system_path = 'Systems'
            self.chassis_path = 'Chassis'
            self.actions['ApplySettings']=self.update_system
            self.actions['Reset']=self.create_system
            self.actions['Subscribe']=self.subscribe_events

        super(RedfishAPI, self).__init__()


    def post(self, path):
        if path.find(self.system_path) != -1 or path.find(self.chassis_path) != -1:
            args = parser.parse_args()
            try:
                action = args['Action']
                path = path.split('/')
                if len(path) >= 2:
                    cs_puid = int(path[1])
                else:
                    cs_puid = 0
                resp = self.actions[action](action, cs_puid)
            except KeyError as e:
                traceback.print_exc()
                resp = error_response('Unknown action: {0}'.format(e), 400)
            except Exception:
                traceback.print_exc()
                resp = INTERNAL_ERROR
        elif path == 'EventService':
            args = parser.parse_args()
            try:
                action = args['Action']
                resp = self.actions[action](action)
            except KeyError as e:
                traceback.print_exc()
                resp = error_response('Unknown action: {0}'.format(e), 400)
            except Exception:
                traceback.print_exc()
                resp = INTERNAL_ERROR
        else:
            resp = '', 404
        return resp

        """
        Either return ServiceRoot or let resource manager handel
        """
    def get(self, path=None):

        try:
            if path is not None:
                # path has a value
                config = self.get_configuration(resource_manager, path)
            else:
                # path is None, fetch ServiceRoot
                config = resource_manager.configuration
            resp = config, 200
        except PathError:
            resp = error_response('Attribute Does Not Exist', 404)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp


    def delete(self, path):
        """
        Delete pooled node -- ONLY ALLOWS THE DELETION OF A POOLED NODE
        """
        try:
            path = path.split('/')
            assert len(path) == 2
            assert path[0] == self.system_path
            cs_puid = int(path[1])
            resource_manager.remove_pooled_node(cs_puid)
            resp = {'Message': 'Pooled node deleted successfully'}, 200
        except (AssertionError, ValueError):
            resp = error_response('Unknown DELETE request - this is only for pooled nodes', 404)
        except RemovePooledNodeError as e:
            resp = error_response(e, 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    @staticmethod
    def create_system(action, idx=1):
        global resource_manager
        try:
            assert request.json is not None, 'No JSON configuration given'
            if(action =='Reset'):
                cs_puid = idx
                config = resource_manager.update_cs(cs_puid,request.json)
            else:
                config = resource_manager._create_redfish(request.json, action)
            resp = config, 201
        except CreatePooledNodeError as e:
            resp = error_response(e, 406)
        except AssertionError as e:
            resp = error_response(e, 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    @staticmethod
    def update_system(action, idx=1):
        global resource_manager
        try:
            assert request.json is not None, 'No JSON configuration given'

            config = resource_manager.update_system(request.json, idx)
            resp = config, 201
        except CreatePooledNodeError as e:
            resp = error_response(e, 406)
        except AssertionError as e:
            resp = error_response(e, 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    @staticmethod
    def subscribe_events(action, idx=1):
        global resource_manager
        try:
            assert request.json is not None, 'No JSON configuration given'

            config = resource_manager.add_event_subscription(request.json)
            resp = config, 201
        except CreatePooledNodeError as e:
            resp = error_response(e, 406)
        except AssertionError as e:
            resp = error_response(e, 400)
        except Exception:
            traceback.print_exc()
            resp = INTERNAL_ERROR
        return resp

    @staticmethod
    def get_configuration(obj, path):
        """
        Helper function to follow the given path starting with the given object

        Arguments:
            obj  - Beginning object to start searching down.  obj should have a get_resource()
            path - Path of object to get

        """

        try:
            config = obj.get_resource(path)
        except (IndexError, AttributeError, TypeError, AssertionError, KeyError) as e:
            traceback.print_exc()
            raise PathError("Resource not found: {}".format(e))
        # print (config)      # Print out static objects
        return config

#
# If DELETE /redfish/v1/reset, then reset the resource manager
#
@g.app.route('/redfish/v1/reset/', methods=['DELETE'])
def reset():
    try:
        init_resource_manager()
        data = {'Message': 'Emulator reset successfully'}
        resp = json.dumps(data, indent=4), 200
    except Exception:
        traceback.print_exc()
        resp = error_response('Internal Server Error', 500, True)
    return resp


#
# If GET /, then return index.html (an intro page)
#
@g.app.route('/')
def index():
    return render_template('index.html')

@g.app.route('/redfish')
def serviceInfo():
    return render_template('service.json')

@g.app.route('/browse.html')
def browse():
    return render_template('browse.html')

# Return metadata as type text/xml
@g.app.route('/redfish/v1/$metadata')
def get_metadata():
    logging.info ('In get_metadata')
    try:

        md_xml = ""

        if os.path.exists('Resources/$metadata/index.xml'):
            # Use dynamic data source
            filename = 'Resources/$metadata/index.xml'
        else:
            # Use static mockup
            mockup_path = MOCKUPFOLDERS[0]
            filename = os.path.join("api_emulator", mockup_path, 'static', '$metadata', 'index.xml')

        with open(filename, 'r') as var:
            for line in var:
                line = line.rstrip()
                md_xml += line

        resp = make_response(md_xml, 200)
        resp.headers['Content-Type'] = 'text/xml'
        return resp

    except Exception:
        traceback.print_exc()
        resp = error_response('Internal Server Error', 500, True)
    return resp

# Return odata
@g.app.route('/redfish/v1/odata')
def get_odata():
    logging.info ('In get_odata')
    try:

        odata_json = ""

        if os.path.exists('Resources//odata//index.json'):
            # Use dynamic data source
            filename = 'Resources/odata/index.json'
            logging.info ('Resources path exists:', filename)
        else:
            # Use static mockup
            mockup_path = MOCKUPFOLDERS[0]
            filename = os.path.join("api_emulator", mockup_path, 'static', 'odata', 'index.json')

        with open(filename, 'r') as var:
            for line in var:
                line = line.rstrip()
                odata_json += line

        resp = make_response(odata_json, 200)
        resp.headers['Content-Type'] = 'application/json'
        return resp

    except Exception:
        traceback.print_exc()
        resp = error_response('Internal Server Error', 500, True)
    return resp


#
# If any other RESTful request, send to RedfishAPI object for processing. Note: <path:path> specifies any path
#
g.api.add_resource(RedfishAPI, '/redfish/v1/', '/redfish/v1/<path:path>')

#
#
def startup():

    init_resource_manager()

#
# Main method
#
# Determines execution configuration by reading the configuration file and interogating the command line options
#   TRAYS = Specifies the directory from which the resource pools are created.
#   MODE =  Specifies whether the emulator is running locally as a standalone or remotely on a Cloud Foundry instance.
#   HTTPS = Specifies whether the emulator supports "http" or "https"
#   SPEC =  The emulator may support multiple specifications or revisions of a specification.
#           This flag specifies the specification/version to which to conform
#   MOCKUPFOLDERS = This parameter will supercede SPEC.  Specifies a list of
#           folder which contain mockup files in ./static.  For example, if the
#           list contains ["Redfish", "Swordfish"], the files in
#           ./Redfish/static and ./Swordfish/static will be used.  This
#           parameter allows multiple mockup folders to co-exist, and the user
#           can set this parameter to determine which mockups are actually
#           loaded into the emulator.
#
# Passes control to startup()
#
def main():
    global app
    global MODE
    global TRAYS
    global MOCKUPFOLDERS
    global SPEC

    # Open the emulator configuration file
    with open(CONFIG, 'r') as f:
        config = json.load(f)

    HTTPS = config['HTTPS']
    assert HTTPS.lower() in ['enable', 'disable'], 'Unknown HTTPS setting:' + HTTPS

    try:
        TRAYS = config['TRAYS']
    except:
        pass

    try:
        MOCKUPFOLDERS = config['MOCKUPFOLDERS']
        logging.info('Mockup folders')
        g.staticfolders = copy.copy(MOCKUPFOLDERS)
        print (g.staticfolders)
    except:
        pass

    try:
        SPEC = config['SPEC']
        assert SPEC == 'Redfish', 'Unknown spec: {0}, must be Redfish'.format(SPEC)
#    assert SPEC.lower() in ['redfish'], 'Unknown spec: ' + SPEC
    except:
        pass

    MODE = config['MODE']
    assert MODE.lower() in ['local', 'cloud'], 'Unknown mode: ' + MODE
    if(MODE=='Cloud'):
        port = int(os.getenv("PORT"))

    argparser = argparse.ArgumentParser(
    description='Redfish Manageability Interface Emulator - Version: ' + __version__,
    epilog='Developed by Intel')

    if(MODE=='Cloud'):
        argparser.add_argument('-port', type=int, default=port, help='Port to run the emulator on. Port defined by Foundry')
    elif(MODE=='Local'):
        argparser.add_argument('-port', type=int, default=5000, help='Port to run the emulator on. Default is 5000')
        print (' * Redfish endpoint at localhost:5000')

    argparser.add_argument('-debug', action='store_true', default=False,
                           help='Run the emulator in debug mode. Note that if you'
                                ' run in debug mode, then the emulator will only'
                                'be ran locally.')
    args = argparser.parse_args()

    try:
        startup()
    except ConfigurationError as e:
        print('Error Loading Trays: {}'.format(e))
    else:
        if (HTTPS == 'Enable'):
            print (' * Use HTTPS')
            context = ('server.crt', 'server.key')
            kwargs = {'debug': args.debug, 'port': args.port, 'ssl_context' : context}
        else:
            print (' * Use HTTP')
            kwargs = {'debug': args.debug, 'port': args.port}

        if not args.debug:
            kwargs['host'] = '0.0.0.0'

        print (' * Running in', SPEC, 'mode')
        g.app.run(**kwargs)

if __name__ == '__main__':

    main()
else:
    startup()
