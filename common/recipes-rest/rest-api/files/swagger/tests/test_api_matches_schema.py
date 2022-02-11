import glob
import os
import re
import sys

import requests
from flex.core import load, validate, validate_api_call
from flex.exceptions import ValidationError


dir_path = os.path.dirname(os.path.realpath(__file__))
platform = sys.argv[1]
base_url = sys.argv[2]
paths = glob.glob(
    os.path.join(dir_path, "../", "definitions", platform) + "/**/*.yml", recursive=True
)

exitcode = 0

for path in paths:
    schema = load(path)
    validate(schema)
    matches = re.match(".*/api(?P<path>.+)[.]yml$", path)
    if not matches:
        continue

    uri = "/api" + matches.group("path")
    print("testing endpoint %s... " % uri, end="", flush=True)
    response = requests.get(base_url + uri, verify=False)

    try:
        validate_api_call(schema, raw_request=response.request, raw_response=response)
    except ValidationError as ve:
        print("failed\n\n" + str(ve) + "\n")
        exitcode = 1
    else:
        print("okay")

sys.exit(exitcode)
