#!/usr/bin/env python3
import json
import os
import sys

import jsonschema.validators


def validator(schema_path):
    base = "file://{}/".format(os.path.split(os.path.realpath(schema_path))[0])
    with open(schema_path) as f:
        schema = json.load(f)
    resolver = jsonschema.RefResolver(base, schema)
    validator = jsonschema.Draft4Validator(schema, resolver=resolver)
    return validator


def validate_json_conf(schema, js):
    v = validator(schema)
    try:
        with open(js) as f:
            data = json.load(f)
            v.validate(data)
        return True
    except jsonschema.exceptions.ValidationError as e:
        print("Validation failed", file=sys.stderr)
        print(e, file=sys.stderr)
        return False


def main(name, args):
    if len(args) < 2:
        print("USAGE: {} SCHEMA JSON".format(name))
        sys.exit(1)
    schema = args[0]
    files = args[1:]
    for f in files:
        if validate_json_conf(schema, f):
            print("SUCCESS: ", f)
        else:
            print("FAILURE: ", f, file=sys.stderr)
            sys.exit(1)


if __name__ == "__main__":
    main(sys.argv[0], sys.argv[1:])
