import os
import uuid
from uuid import getnode as get_mac
import fcntl
import struct
import socket
from subprocess import *
from collections import OrderedDict
from node import node
from node_logs import get_node_logs
from rest_pal_legacy import pal_get_uuid, pal_get_eth_intf_name

SIOCGIFADDR    = 0x8915          # get PA address


def get_fqdn_str():
    fqdn_str = ""
    file_path = "/etc/hosts"
    if os.path.isfile(file_path):
        fqdn_str = open(file_path).readline()
        fqdn_str = fqdn_str.strip().split()[1]
    return fqdn_str


def get_mac_address(if_name):
    mac_addr = ""
    mac_path = "/sys/class/net/%s/address" % (if_name)
    if os.path.isfile(mac_path):
        mac = open(mac_path).read()
        mac_addr = mac[0:17].upper()
    else:
        mac = get_mac()
        mac_addr = ":".join(("%012X" % mac)[i : i + 2] for i in range(0, 12, 2))
    return mac_addr


def get_ipv4_ip_address(if_name):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        v4_ip = socket.inet_ntoa(fcntl.ioctl(
                s.fileno(),
                SIOCGIFADDR,
                struct.pack('256s', if_name[:15].encode('utf-8')))[20:24])
    except IOError:
        return None

    return v4_ip


def get_ipv4_netmask(if_name):
    ipv4_netmask = ""
    cmd = "/sbin/ifconfig %s |  awk '/Mask/{print $4}' " % if_name
    ipv4_netmask = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    if ipv4_netmask:
        ipv4_netmask = ipv4_netmask.strip().split(':')[1]
    return ipv4_netmask


def get_ipv6_ip_address(if_name):
    v6_ip = ""
    cmd = "ip -6 -o addr show dev %s scope global | awk '{print $4}'"  % if_name
    v6_ip = Popen(cmd, shell=True, stdout=PIPE).stdout.read().decode()
    if v6_ip:
        v6_ip = v6_ip.strip().split('/64')[0]
    return v6_ip


def get_ipv4_gateway():
    """ Returns the default gateway """
    octet_list = []
    gw_from_route = None
    f = open ('/proc/net/route', 'r')
    for line in f:
        words = line.split()
        dest = words[1]
        try:
            if (int (dest) == 0):
                gw_from_route = words[2]
                break
        except ValueError:
            pass
    if not gw_from_route:
        return None

    for i in range(8, 1, -2):
        octet = gw_from_route[i-2:i]
        octet = int(octet, 16)
        octet_list.append(str(octet))

    gw_ip = ".".join(octet_list)

    return gw_ip


def get_managers():
    body = {}
    try:
        body = {
            "@odata.context":"/redfish/v1/$metadata#ManagerCollection.ManagerCollection",
            "@odata.id": "/redfish/v1/Managers",
            "@odata.type": "#ManagerCollection.ManagerCollection",
            "Name": "Manager Collection",
            "Members@odata.count": 1,
            "Members": [{"@odata.id": "/redfish/v1/Managers/1"}]
        }
    except Exception as error:
        print(error)
    return node(body)


def get_managers_members():
    body = {}
    try:
        uuid_data = pal_get_uuid()
        obc_version = Popen("cat /etc/issue", shell=True, stdout=PIPE).stdout.read().decode()
        obc_version = obc_version.rstrip('\n')
        body = {
            "@odata.context": "/redfish/v1/$metadata#Manager.Manager",
            "@odata.id": "/redfish/v1/Managers/1",
            "@odata.type": "#Manager.v1_1_0.Manager",
            "ManagerType": "BMC",
            "UUID": str(uuid_data),
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "FirmwareVersion": str(obc_version),
            "NetworkProtocol": {
                "@odata.id":"/redfish/v1/Managers/1/NetworkProtocol"
            },
            "EthernetInterfaces": {
                "@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces"
            },
            "LogServices": {
                "@odata.id": "/redfish/v1/Managers/1/LogServices"
            },
            "Links": {},
            "Actions": {
                "#Manager.Reset": {
                    "target": "/redfish/v1/Managers/1/Actions/Manager.Reset",
                    "ResetType@Redfish.AllowableValues": [
                        "ForceRestart",
                        "GracefulRestart"
                    ]
                }
            }
        }
    except Exception as error:
        print(error)
    return node(body)


def get_manager_ethernet():
    body = {}
    try:
        body = {
            "@odata.context": "/redfish/v1/$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection",
            "@odata.id": "/redfish/v1/Managers/System/EthernetInterfaces",
            "@odata.type": "#EthernetInterfaceCollection.EthernetInterfaceCollection",
            "Name": "Ethernet Network Interface Collection",
            "Description": "Collection of EthernetInterfaces for this Manager",
            "Members@odata.count": 1,
            "Members": [{
                "@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces/1"
            }]
        }
    except Exception as error:
        print(error)
    return node(body)


def get_ethernet_members():
    body = {}
    mac_addr = ""
    host_name = ""
    fqdn = ""
    ipv4_ip = ""
    ipv4_netmask = ""
    ipv4_gateway = ""
    ipv6_ip = ""
    try:
        eth_intf = pal_get_eth_intf_name()
        mac_addr = get_mac_address(eth_intf)
        host_name = socket.gethostname()
        fqdn = get_fqdn_str()
        ipv4_ip = get_ipv4_ip_address(eth_intf)
        ipv4_netmask = get_ipv4_netmask(eth_intf)
        ipv4_gateway = get_ipv4_gateway()
        ipv6_ip = get_ipv6_ip_address(eth_intf)

        body = {
            "@odata.context": "/redfish/v1/$metadata#EthernetInterface.EthernetInterface",
            "@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces/1",
            "@odata.type": "#EthernetInterface.v1_4_0.EthernetInterface",
            "Id": "1",
            "Name": "Manager Ethernet Interface",
            "Description": "Management Network Interface",
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "InterfaceEnabled": bool(1),
            "MACAddress": str(mac_addr),
            "SpeedMbps": 100,
            "HostName": str(host_name),
            "FQDN": str(fqdn),
            "IPv4Addresses": [{
                "Address": str(ipv4_ip),
                "SubnetMask": str(ipv4_netmask),
                "AddressOrigin": "DHCP",
                "Gateway": str(ipv4_gateway)
            }],
            "IPv6Addresses": [{
                "Address": str(ipv6_ip),
                "PrefixLength": 64,
                "AddressOrigin": "SLAAC",
                "AddressState": "Preferred"
            }],
            "StaticNameServers": [],
        }
    except Exception as error:
        print(error)
    return node(body)


def get_manager_network():
    body = {}
    host_name = ""
    fqdn = ""
    try:
        host_name = socket.gethostname()
        fqdn = get_fqdn_str()
        true_flag = bool(1)
        body = {
            "@odata.context": "/redfish/v1/$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol",
            "@odata.id": "/redfish/v1/Managers/1/NetworkProtocol",
            "@odata.type": "#ManagerNetworkProtocol.v1_2_0.ManagerNetworkProtocol",
            "Id": "NetworkProtocol",
            "Name": "Manager Network Protocol",
            "Description": "Manager Network Service Status",
            "Status": {
	            "State": "Enabled",
	            "Health": "OK"
            },
            "HostName": str(host_name),
            "FQDN": str(fqdn),
            "HTTP": {
	            "ProtocolEnabled": true_flag,
	            "Port": 8080
            },
            "HTTPS": {
	            "ProtocolEnabled": true_flag,
	            "Port": 8443
            },
            "SSH": {
	            "ProtocolEnabled": true_flag,
	            "Port": 22
            },
            "SSDP": {
                    "ProtocolEnabled": true_flag,
                    "Port": 1900,
                    "NotifyMulticastIntervalSeconds": 600,
                    "NotifyTTL": 5,
                    "NotifyIPv6Scope": "Site"
            }
        }
    except Exception as error:
        print(error)
    return node(body)

# /redfish/v1/Managers/1/LogService
def get_manager_logservice(list):
    body = {}
    mem_list = []

    try:
        for fru in list:
            mem = {
                "@odata.id": f"/redfish/v1/Managers/1/LogServices/{fru}"
            }
            mem_list.append(mem)

        body = {
            "@odata.context": "/redfish/v1/$metadata#LogServiceCollection.LogServiceCollection",
            "@odata.id": "/redfish/v1/Managers/1/LogServices",
            "@odata.type": "#LogServiceCollection.LogServiceCollection",
            "Members@odata.count": len(list),
            "Members": mem_list
        }
    except Exception as error:
        print(error)
    return node(body)

# /redfish/v1/Managers/1/LogService/psu1
def get_logservice_members(id):
    body = {}

    # OverWritePolicy: ["Unknown", "WrapsWhenFull", "NeverOverWrites"]
    try:
        body = {
            "@odata.id": f"/redfish/v1/Managers/1/LogServices/{id}",
            "@odata.type": "#LogService.v1_2_0.LogService",
            "Id": str(id),
            "Name": "System Log Service",
            "MaxNumberOfRecords": 1000,
            "OverWritePolicy": "Unknown",
            "DateTime": "",
            "DateTimeLocalOffset": "",
            "Status": {
                "State": "Enabled",
                "Health": "OK"
            },
            "Actions": {
                "#LogService.ClearLog": {
                    "target": f"/redfish/v1/Managers/1/LogServices/{id}/Actions/LogService.ClearLog"
                }
            },
            "Entries": {
                "@odata.id": f"/redfish/v1/Managers/1/LogServices/{id}/Entries",
            }
        }
    except Exception as error:
        print(error)

    return node(body)

# /redfish/v1/Managers/1/LogService/fru/Entries
def get_log_entries(id):
    max_entry = 5
    mem_list = []

    try:
        logs = get_node_logs(str(id)).getInformation()
        entries = logs["Logs"]
        exist_entry = len(entries)

        if exist_entry < 1:
            max_entry = 0
        else:
            max_entry = min(exist_entry, max_entry)

        if max_entry != 0:    
            for index in range(max_entry):
                mem = get_log_entries_member(str(id), index, entries)
                mem_list.append(mem)

        body = {
            "@odata.id": f"/redfish/v1/Managers/1/LogServices/{id}/Entries",
            "@odata.type": "#LogEntryCollection.LogEntryCollection",
            "Name": "Log Service Collection",
            "Description": "Collection of Logs for this System",
            "Members@odata.count": max_entry,
            "Members": mem_list
        }
    except Exception as error:
        print(error)

    return node(body)


def get_log_entries_member(fru, index, entries):
    body = {}
    list_len = len(entries)
    rev_index = (list_len - 1) - index 
    try:
        app_name = entries[rev_index]['APP_NAME']
        fru_id = entries[rev_index]['FRU#']
        fru_name = entries[rev_index]['FRU_NAME']
        msg = entries[rev_index]['MESSAGE']
        create = entries[rev_index]['TIME_STAMP']
        
        body = OrderedDict([
            ("@odata.id",
             f"/redfish/v1/Managers/1/LogServices/{fru}/Entries/{index+1}"),
            ("@odata.type",  "#LogEntry.v1_8_0.LogEntry"),
            ("Id", index+1),
            ("Name",  f"{fru_name} ({app_name})"),
            ("Created",  str(create)),
            ("Message",  str(msg)),
            ("EntryType", "Event")
        ])

    except Exception as error:
        print(error)

    return body


# /redfish/v1/Managers/1/LogService/fru/Actions
def get_log_actions(id):
    body = {}

    try:
        body = {
            "#LogService.ClearLog": {
                "target": f"/redfish/v1/Managers/1/LogServices/{id}/Actions/LogService.ClearLog"
            }
        }
        
    except Exception as error:
        print(error)

    return node(body)
