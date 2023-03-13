# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Redfish Processors and Processor Resources. Based on version 1.0.0

from flask_restful import Resource
from .templates.processor import format_processor_template
import logging

members = {}
INTERNAL_ERROR = 500


class Processor(Resource):
    """
    Processor.1.0.0.Processor
    """

    def __init__(self, **kwargs):
        pass

    def get(self, ident1, ident2):
        resp = 404
        if ident1 not in members:
            return 'not found',404
        if ident2 not in members[ident1]:
            return 'not found',404
        return members[ident1][ident2], 200


class Processors(Resource):
    """
    Processor.1.0.0.ProcessorCollection
    """

    def __init__(self, rb, suffix):
        """
        Processors Constructor
        """
        self.config = {u'@odata.context': '{rb}$metadata#ProcessorCollection.ProcessorCollection'.format(rb=rb),
                       u'@odata.id': '{rb}{suffix}'.format(rb=rb, suffix=suffix),
                       u'@odata.type': u'#ProcessorCollection.ProcessorCollection'}

    def get(self, ident):
        try:
            if ident not in members:
                return 404
            procs = []
            for p in members.get(ident, {}).values():
                procs.append({'@odata.id': p['@odata.id']})
            self.config['@odata.id']='{prefix}/{ident}/Processors'.format(prefix=self.config['@odata.id'],ident=ident)
            self.config['Members'] = procs
            self.config['Members@odata.count'] = len(procs)
            resp = self.config, 200
        except Exception as e:
            logging.error(e)
            resp = 'internal error', INTERNAL_ERROR
        return resp


def CreateProcessor(**kwargs):
    suffix_id = kwargs['suffix_id']
    processor_id = kwargs['processor_id']
    if suffix_id not in members:
        members[suffix_id] = {}
    members[suffix_id][processor_id] = format_processor_template(**kwargs)
