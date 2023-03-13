# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Redfish Emulator Role Service.
#   Temporary version, to be removed when AccountService goes dynamic

class AccountService(object):

    def __init__(self):
        self._accounts = { 'Administrator': 'Password',
                           'User': 'Password' }
        self._roles = { 'Administrator': 'Admin',
                        'User': 'ReadOnlyUser' }

    def checkPriviledgeLevel(self, user, level):
        if self._roles[user] == level:
            return True
        else:
            return False

    def getPassword(self, username):
        if username in self._accounts:
            return self._accounts[username]
        else:
            return None

    def checkPrivilege(self, privilege, username, errorResponse):
        def wrap(func):
            def inner(*args, **kwargs):
                if self.checkPriviledgeLevel(username(), privilege):
                    return func(*args, **kwargs)
                else:
                    return errorResponse()
            return inner
        return wrap
