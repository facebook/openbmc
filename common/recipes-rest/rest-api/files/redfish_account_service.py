from node import node

def get_account_service():
    body = {}
    try:
       body = {
            "@odata.context": "/redfish/v1/$metadata#AccountService.AccountService",
	    "@odata.id": "/redfish/v1/AccountService",
	    "@odata.type": "#AccountService.v1_0_0.AccountService",
	    "Id": "AccountService",
	    "Name": "Account Service",
	    "Description": "Account Service",
	    "Accounts": {
		    "@odata.id": "/redfish/v1/AccountService/Accounts"
	    }
       }
    except Exception as error:
        print(error)
    return node(body)
