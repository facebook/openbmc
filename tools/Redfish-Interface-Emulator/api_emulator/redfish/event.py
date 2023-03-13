# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Event resource

import strgen

class Event():
    """
    Redfish event class
    """
    def __init__(self, eventType="Alert", severity="Warning", message="",
                 messageID="Alert.1.0", messageArgs=[], originOfCondition="",
                 context="Default"):
        """
        Event constructor
        """
        self.config = {
            "EventType": eventType,
            "Severity": severity,
            "Message": message,
            "MessageID": messageID,
            "MessageArgs": messageArgs,
            "OriginOfCondition": originOfCondition,
            "Context": context
        }

    @property
    def configuration(self):
        config = self.config.copy()
        config['EventID'] = strgen.StringGenerator('[A-Z]{3}[0-9]{10}').render()
        return config
