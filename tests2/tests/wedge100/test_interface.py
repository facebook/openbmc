import unittest
from common.base_interface_test import CommonInterfaceTest

class InterfaceTest(CommonInterfaceTest):

    def test_usb0_v6_interface(self):
        '''
        Tests eth0 v6 interface
        '''
        self.set_ifname("usb0")
        self.log_check(name=__name__)
        self.assertEqual(self.ping_v6(), 0,
                         'Ping test for usb0 v6 failed')

if __name__ == '__main__':
    test_suite = unittest.TestLoader().loadTestsFromTestCase('InterfaceTest')
    unittest.TextTestRunner().run(test_suite)
