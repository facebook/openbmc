# Copyright Notice:
# Copyright 2016-2021 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link:
# https://github.com/DMTF/python-redfish-library/blob/master/LICENSE.md

# The redfish support comes from the https://github.com/DMTF/python-redfish-library
# package.  It must be installed on the devserver before running this script.

import redfish
import json


redfishTop = "/redfish/v1"
dumpfile = "rfData.json"

# When running remotely connect using the address, account name,
# and password to send https requests.
# Manually change the loginHost and credentials below based on the
# expected target of LF or Meta oBMC.  LF host expects to use port
# 80 and Meta Redfish expects to use port 8443.
#
# LF-oBMC Redfish
#        loginHost = "https://sled2403bo.02.snc8.facebook.com"
# Meta-oBMC Redfish
# Change <target_system> to the URL for the target system.  An
# example would be https://sled2403bo.02.snc8.facebook.com:8443.
loginHost = "<target_system>:8443"

loginAccount = "root"
# TODO - Accept password from user to support non-lab system.
loginPassword = "0penBmc"


class loginToRedfishServer:
    def __init__(self):
        self.loggedIn = False
        self.loginObj = None

    def login(self):
        if self.loggedIn is True:
            print("Already logged in")
            return

        ## Create a REDFISH object
        try:
            self.loginObj = redfish.redfish_client(
                base_url=loginHost,
                username=loginAccount,
                password=loginPassword,
                default_prefix=redfishTop,
            )
            # Login into the server and create a session
            self.loginObj.login(auth="basic")
        #            self.loginObj.login(auth="session")
        except:
            print("Unable to login to server")
            return False, None
        self.loggedIn = True
        return True, self.loginObj

    def logout(self):
        # Logout of the current session
        if self.loggedIn is False:
            return False
        self.loginObj.logout()
        self.loggedIn = False
        return True


class dumpRedfishData:
    def __init__(self):
        self.loginObj = None
        self.rfDataList = [redfishTop]
        self.rfData = None
        self.rfObj = None
        self.path = redfishTop
        self.loggedIn = False

    def loginToServer(self):
        self.loginObj = loginToRedfishServer()
        retCode, self.rfObj = self.loginObj.login()
        if retCode is False:
            self.loggedIn = False
            return False
        self.loggedIn = True

    def logoutFromServer(self):
        if self.loggedIn is True:
            retCode = self.loginObj.logout()
            if retCode is True:
                self.loggedIn = False

    def getRedfishDataFromServer(self):
        if self.loggedIn is False:
            result = self.loginToServer()
            if result is False:
                self.rfdata = {}
                return result
        if self.rfObj is None or self.path is None:
            print("ERROR: Unable to get data from server, invalid parameter")
            self.rfData = {}
            return False
        try:
            # Do a GET on a given path
            response = self.rfObj.get(self.path, None)
            # reason = response._http_response.reason
            status_code = response._http_response.status_code
            header = response._http_response.headers
            body = response._http_response.json()
            self.rfData = body
            if int(status_code) == 200 and int(header["Content-Length"]) > 0:
                return True
        except:
            print("ERROR: REST interface returned an error")
        return False

    def getKeys(self, data):
        for key, value in data.items():
            yield key, value
            if isinstance(value, dict):
                yield from self.getKeys(value)
            if isinstance(value, list) and bool(value):
                for x in value:
                    if isinstance(x, dict):
                        yield from self.getKeys(x)

    def parseData(self, data):
        if data is None:
            return
        for key, value in self.getKeys(data):
            if key == "@odata.id" and value not in self.rfDataList:
                self.rfDataList.append(value)

    def getRedfishData(self):
        i = 0
        rfDict = {}
        while i < len(self.rfDataList):
            self.path = self.rfDataList[i]
            print(f"List iteration #{i} for path {self.path}")
            retCode = self.getRedfishDataFromServer()
            if self.rfData is not None:
                self.parseData(self.rfData)
                rfDict.update({self.path: self.rfData})
            i += 1
        self.logoutFromServer()
        return rfDict


def main():
    rf = dumpRedfishData()
    rfData = rf.getRedfishData()
    if rfData is None:
        return 1
    outFile = open(dumpfile, "w")
    json.dump(rfData, outFile, indent=4, sort_keys=True)
    outFile.close()
    return 0


if __name__ == "__main__":
    main()
