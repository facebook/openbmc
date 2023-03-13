# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

import copy

_TEMPLATE = \
{
	"@Redfish.Copyright": "Copyright 2014-2016 DMTF. All rights reserved.",
	"@odata.context": "{rb}$metadata#ActionInfo.ActionInfo",
	"@odata.id": "{rb}Systems/{sys_id}/ResetActionInfo",
	"@odata.type": "#ActionInfo.v1_0_0.ActionInfo",
	"Parameters": [{
		"Name": "ResetType",
		"Required": "true",
		"DataType": "String",
		"AllowableValues": [
			"On",
			"ForceOff",
			"GracefulShutdown",
			"GracefulRestart",
			"ForceRestart",
			"Nmi",
			"ForceOn",
			"PushPowerButton"
		]
	}]
}
def get_ResetActionInfo_instance(wildcards):

    c = copy.deepcopy(_TEMPLATE)
    print (wildcards)
    c['@odata.context']=c['@odata.context'].format(**wildcards)
    c['@odata.id']=c['@odata.id'].format(**wildcards)

    return c
