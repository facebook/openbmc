import asyncio
import unittest
from unittest.mock import Mock, mock_open

import aiohttp.web
from aiohttp.test_utils import AioHTTPTestCase, unittest_run_loop
from common_middlewares import jsonerrorhandler


class TestManagerService(AioHTTPTestCase):
    async def setUpAsync(self):
        asyncio.set_event_loop(asyncio.new_event_loop())
        file_contents_dict = {
            "/etc/issue": "OpenBMC Release fby2-v2020.49.2",
            "/etc/hosts": "x localhost.localdomain",
            # prefix 'x' so the logic in get_fqdn_str() can process this
        }

        self.patches = [
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_platform_name",
                return_value="FBY2",
            ),
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_uuid",
                new_callable=unittest.mock.MagicMock,
                return_value="bd7e0200-8227-3a1c-30c0-286261016903",
            ),
            unittest.mock.patch(
                "builtins.open",
                self.mapped_mock_open(file_contents_dict),
            ),
            unittest.mock.patch(
                "socket.gethostname",
                return_value="sled0502ju-oob.06.ftw3.facebook.com",
            ),
            unittest.mock.patch(
                "rest_pal_legacy.pal_get_eth_intf_name",
                new_callable=unittest.mock.MagicMock,
                return_value="eth_intf",  # just a placeholder
            ),
            unittest.mock.patch(
                "redfish_managers.get_mac_address",
                return_value="98:03:9B:A4:D6:3F",
            ),
            unittest.mock.patch(
                "redfish_managers.get_ipv4_ip_address",
                return_value="10.0.2.15",
            ),
            unittest.mock.patch(
                "redfish_managers.get_ipv4_netmask",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "redfish_managers.get_ipv4_gateway",
                return_value="10.0.2.2",
            ),
            unittest.mock.patch(
                "redfish_managers.get_ipv6_ip_address",
                new_callable=unittest.mock.MagicMock,  # python < 3.8 compat
                return_value=asyncio.Future(),
            ),
            unittest.mock.patch(
                "aggregate_sensor.aggregate_sensor_init",
                create=True,
                return_value=None,
            ),
        ]

        for p in self.patches:
            p.start()
            self.addCleanup(p.stop)

        ipv6 = "2401:db00:1130:60ff:face:0:5:0"
        ipv4 = "255.255.255.0"
        import redfish_managers

        redfish_managers.get_ipv4_netmask.return_value.set_result(ipv4)
        redfish_managers.get_ipv6_ip_address.return_value.set_result(ipv6)
        await super().setUpAsync()

    def mapped_mock_open(self, file_contents_dict):
        real_open = open
        mock_files = {}
        for fname, content in file_contents_dict.items():
            mock_files[fname] = mock_open(read_data=content).return_value

        def my_open(fname, *args, **kwargs):
            if fname in mock_files:
                return mock_files[fname]
            else:
                return real_open(fname, *args, **kwargs)

        mock_opener = Mock()
        mock_opener.side_effect = my_open
        return mock_opener

    @unittest_run_loop
    async def test_get_managers(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#ManagerCollection.ManagerCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Managers",
            "@odata.type": "#ManagerCollection.ManagerCollection",
            "Name": "Manager Collection",
            "Members@odata.count": 1,
            "Members": [{"@odata.id": "/redfish/v1/Managers/1"}],
        }
        req = await self.client.request("GET", "/redfish/v1/Managers")
        resp = await req.json()
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_managers_members(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#Manager.Manager",
            "@odata.id": "/redfish/v1/Managers/1",
            "@odata.type": "#Manager.v1_1_0.Manager",
            "ManagerType": "BMC",
            "UUID": "bd7e0200-8227-3a1c-30c0-286261016903",
            "Name": "Manager",
            "Id": "BMC",
            "Status": {"State": "Enabled", "Health": "OK"},
            "FirmwareVersion": "OpenBMC Release fby2-v2020.49.2",
            "NetworkProtocol": {"@odata.id": "/redfish/v1/Managers/1/NetworkProtocol"},
            "EthernetInterfaces": {
                "@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces"
            },
            "LogServices": {"@odata.id": "/redfish/v1/Managers/1/LogServices"},
            "Links": {},
            "Actions": {
                "#Manager.Reset": {
                    "target": "/redfish/v1/Managers/1/Actions/Manager.Reset",
                    "ResetType@Redfish.AllowableValues": [
                        "ForceRestart",
                        "GracefulRestart",
                    ],
                }
            },
        }
        req = await self.client.request("GET", "/redfish/v1/Managers/1")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_manager_ethernet(self):
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#EthernetInterfaceCollection.EthernetInterfaceCollection",  # noqa: B950
            "@odata.id": "/redfish/v1/Managers/System/EthernetInterfaces",
            "@odata.type": "#EthernetInterfaceCollection.EthernetInterfaceCollection",
            "Name": "Ethernet Network Interface Collection",
            "Description": "Collection of EthernetInterfaces for this Manager",
            "Members@odata.count": 1,
            "Members": [{"@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces/1"}],
        }
        req = await self.client.request(
            "GET", "/redfish/v1/Managers/1/EthernetInterfaces"
        )
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_manager_network(self):
        true_flag = bool(1)
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#ManagerNetworkProtocol.ManagerNetworkProtocol",  # noqa: B950
            "@odata.id": "/redfish/v1/Managers/1/NetworkProtocol",
            "@odata.type": "#ManagerNetworkProtocol.v1_2_0.ManagerNetworkProtocol",
            "Id": "NetworkProtocol",
            "Name": "Manager Network Protocol",
            "Description": "Manager Network Service Status",
            "Status": {"State": "Enabled", "Health": "OK"},
            "HostName": "sled0502ju-oob.06.ftw3.facebook.com",
            "FQDN": "localhost.localdomain",
            "HTTP": {"ProtocolEnabled": true_flag, "Port": 8080},
            "HTTPS": {"ProtocolEnabled": true_flag, "Port": 8443},
            "SSH": {"ProtocolEnabled": true_flag, "Port": 22},
            "SSDP": {
                "ProtocolEnabled": true_flag,
                "Port": 1900,
                "NotifyMulticastIntervalSeconds": 600,
                "NotifyTTL": 5,
                "NotifyIPv6Scope": "Site",
            },
        }
        req = await self.client.request("GET", "/redfish/v1/Managers/1/NetworkProtocol")
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    @unittest_run_loop
    async def test_get_ethernet_members(self):
        true_flag = bool(1)
        expected_resp = {
            "@odata.context": "/redfish/v1/$metadata#EthernetInterface.EthernetInterface",  # noqa: B950
            "@odata.id": "/redfish/v1/Managers/1/EthernetInterfaces/1",
            "@odata.type": "#EthernetInterface.v1_4_0.EthernetInterface",
            "Id": "1",
            "Name": "Manager Ethernet Interface",
            "Description": "Management Network Interface",
            "Status": {"State": "Enabled", "Health": "OK"},
            "InterfaceEnabled": true_flag,
            "MACAddress": "98:03:9B:A4:D6:3F",
            "SpeedMbps": 100,
            "HostName": "sled0502ju-oob.06.ftw3.facebook.com",
            "FQDN": "localhost.localdomain",
            "IPv4Addresses": [
                {
                    "Address": "10.0.2.15",
                    "SubnetMask": "255.255.255.0",
                    "AddressOrigin": "DHCP",
                    "Gateway": "10.0.2.2",
                }
            ],
            "IPv6Addresses": [
                {
                    "Address": "2401:db00:1130:60ff:face:0:5:0",
                    "PrefixLength": 64,
                    "AddressOrigin": "SLAAC",
                    "AddressState": "Preferred",
                }
            ],
            "StaticNameServers": [],
            "NameServers": [],
            "LinkStatus": "LinkUp",
        }
        req = await self.client.request(
            "GET", "/redfish/v1/Managers/1/EthernetInterfaces/1"
        )
        resp = await req.json()
        self.maxDiff = None
        self.assertEqual(resp, expected_resp)
        self.assertEqual(req.status, 200)

    async def get_application(self):
        webapp = aiohttp.web.Application(middlewares=[jsonerrorhandler])
        from redfish_common_routes import Redfish

        redfish = Redfish()
        redfish.setup_redfish_common_routes(webapp)
        return webapp
