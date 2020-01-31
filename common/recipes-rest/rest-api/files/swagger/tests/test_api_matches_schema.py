import glob
import os
import re
import sys

import requests
from flex.core import load, validate, validate_api_call


dir_path = os.path.dirname(os.path.realpath(__file__))
platform = sys.argv[1]
base_url = sys.argv[2]
paths = glob.glob(
    os.path.join(dir_path, "../", "definitions", platform) + "/**/*.yml", recursive=True
)

for path in paths:
    schema = load(path)
    validate(schema)
    matches = re.match(".*/api(?P<path>.+)[.]yml$", path)
    if matches:
        uri = "/api" + matches.group("path")
        print("testing endpoint %s" % uri)
        response = requests.get(base_url + uri, verify=False)
        validate_api_call(schema, raw_request=response.request, raw_response=response)
