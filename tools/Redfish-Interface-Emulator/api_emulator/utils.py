# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Utilities used through out the library
#   timestamp()
#   process_id()
#   check_initialization

import os
import json
import datetime
from functools import wraps


def timestamp():
    """
    Return an ISO timestamp with milliseconds removed
    """
    return datetime.datetime.now().isoformat().split('.')[0]


def process_id(odata_id, base_dir, rest_base):
    """
    Gets the index.html associated with the odata_id
    """
    index_dir = os.path.abspath(os.path.join(
        base_dir, odata_id.split(rest_base)[-1]))
    index_html = os.path.join(index_dir, 'index.json')
    assert os.path.exists(index_html), \
        '"{0}" does not exist'.format(index_html)
    with open(index_html, 'r') as f:
        index = json.load(f)
    return index


def check_initialized(func):
    """
r   Wrapper function to check if the initialized member variable
    has been set to True in a class.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        cls = args[0]
        if cls.initialized:
            raise RuntimeError('Object has already been initialized')
        return func(*args, **kwargs)
    return wrapper

def replace_recurse(c, wildcards):
    """
    Replaces wildcard values in 'c' with the replacements specified in
    'wildcards'

    The parameter 'wildcards' is a dictionary of wildcards and their replacement.
    Theoretically, any string can be used as the wildcard.  This code has been
    testing with the wildcards delimite by curly braces (e.g. {id})

    The paramerter 'c' starts as a dictionary, but during the recursion, c can
    become
        - a dictionary - recurse
        - a list - recurse on each list item
        - a string - replace wildcards
        - a float - do nothing
        - a int - do nothing
    """
    # print("recurse c: ", c)
 
    for k, v in c.items():
        if isinstance(v, dict):
            replace_recurse(c[k], wildcards)
        elif isinstance(v, list):
            for index, item in enumerate(v):
                replace_recurse(item, wildcards)
        elif isinstance (v, str):
            # print("key/value : ", k, "; ", v)
            # print("c[k] : ", c[k])
            c[k] = c[k].format(**wildcards)
            # print("c[k]2: ", c[k])
