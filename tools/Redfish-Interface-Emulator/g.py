# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Declares global variables
#
# The canonical way to share information across modules within a single program
# is to create a special module (often called config or cfg). So this file
# should be called config.py.  But too late, now.

from flask import Flask
from flask_restful import Api

# Settings from emulator-config.json
#
staticfolders = []

# Base URI. Will get overwritten in emulator.py
rest_base = 'base'

# Create Flask server
app = Flask(__name__)

# Create RESTful API
api = Api(app)
