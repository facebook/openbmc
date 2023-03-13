# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# get_chassis_template()

#from api_emulator.utils import timestamp

_CHASSIS_TEMPLATE = \
    {
        "@odata.context": "{rb}$metadata#Chassis/Links/Members/$entity",
        "@odata.id": "{rb}Chassis/{id}",
        "@odata.type": "#Chassis.1.0.0.Chassis",
        #"Id": None,
        "Name": "Computer System Chassis",
        "ChassisType": "RackMount",
        "Manufacturer": "Redfish Computers",
        "Model": "3500RX",
        "SKU": "8675309",
        "SerialNumber": "437XR1138R2",
        "Version": "1.02",
        "PartNumber": "224071-J23",
        "AssetTag": "Chicago-45Z-2381",
        "Status": {
            "State": "Enabled",
            "Health": "OK"
        },
        "Links": {
            "ComputerSystems": [
                {
                    "@odata.id": "{rb}Systems/"
                }
            ],
            "ManagedBy": [
                {
                    "@odata.id": "{rb}Managers/1"
                }
            ],
            "ThermalMetrics": {
                "@odata.id": "{rb}Chassis/{id}/ThermalMetrics"
            },
            "PowerMetrics": {
                "@odata.id": "{rb}Chassis/{id}/PowerMetrics"
            },
            "MiscMetrics": {
                "@odata.id": "{rb}Chassis/{id}/MiscMetrics"
            },
            "Oem": {}
        },
        "Oem": {}
    }


def get_chassis_template(rest_base, ident):
    """
    Formats the template

    Arguments:
        rest_base - Base URL for the RESTful interface
        indent    - ID of the chassis
    """
    c = _CHASSIS_TEMPLATE.copy()

    # Formatting
    #c['Id'] = ident
    c['@odata.context'] = c['@odata.context'].format(rb=rest_base)
    c['@odata.id'] = c['@odata.id'].format(rb=rest_base, id=ident)
    c['Links']['ManagedBy'][0]['@odata.id'] = c['Links']['ManagedBy'][0]['@odata.id'].format(rb=rest_base)
    c['Links']['ThermalMetrics']['@odata.id'] = c['Links']['ThermalMetrics']['@odata.id'].format(rb=rest_base, id=ident)
    c['Links']['PowerMetrics']['@odata.id'] = c['Links']['PowerMetrics']['@odata.id'].format(rb=rest_base, id=ident)
    c['Links']['MiscMetrics']['@odata.id'] = c['Links']['MiscMetrics']['@odata.id'].format(rb=rest_base, id=ident)
    c['Links']['ComputerSystems'][0]['@odata.id']=c['Links']['ComputerSystems'][0]['@odata.id'].format(rb=rest_base)
    return c
