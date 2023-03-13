
# HowTo - Develop Redfish Interface Emulator

Copyright 2019-2022 DMTF. All rights reserved.

## About

Redfish Interface Emulator emulates a Redfish Service. The emulation can be for a static resources or dynamic resources.

Static emulation supports HTTP GET requests on the Redfish resource tree. This is similar to the capability of Redfish-Mockup-Server and Redfish-Profile-Simulator. The static emulation capability is accomplished by copying the mockup tree into the xxxx directory and setting a configuration flag to STATIC before starting the emulator.

Dynamic emulation supports HTTP GET, POST, PATCH, DELETE requests on the Redfish resource tree.  The dynamic emulation capability is accomplished by writing Python files that emulated the behavior of a resource.

This document describes the structure of repository and how to code the Python files and place them to emulate a resource. The repository also contains tools that can auto-generated this required Python files.


## Installation

1, Install Python

### Building from Source

```
python setup.py sdist
pip install dist/redfish_utilities-x.x.x.tar.gz
```


## Requirements

`pip install -r requirements.txt`

# Accessing the Redfish Interface Emulator

## Using Postman to send HTTP commands

1.  Start Chrome-Postman

2.  Enter the following path into the URI field

	```
	$> http://localhost:5000/redfish/v1
	```

5.  The HTTP command should be set to GET

6.  Click the 'Send' button

## Testing the Emulator

The emulator can be tested by using the included unit test.  Test can run against the emulator running locally or remotely.

The following is the general command for running unit test. The value of
<SPEC> is Redfish and should correspond to the value in
emulator-config.json used to start the emulator. The value of \<PATH\>
is the path to use for testing. \<PATH\> should be enclosed in
double-quotes.

	$> python unittests.py <SPEC> "<PATH>"

## Run the Unit Test, locally

An instance of the emulator running locally can be tested as follows.
The program unittest.py reads the configuration file to determine the port number to use to communicate with the emulator. 

1.  Start the emulator locally
2.  Execute the following command

	```
	$> python unittests.py Redfish "localhost:5000"
	```

3.  Inspect the log file produced, "test-rsa-emulator.log"

## Running the Unit Test on a remotely hosted 

An instance of the emulator running in the cloud can be test.  The path is the response when the "cf push" is invoked.

1.  Start the emulator remotely
2.  Execute the following command. It the example, the URL to the cloud.

	$> python unittests.py Redfish "rsa-emulator.apps1-fm-int-icloud.intel.com"

3. Inspect the log file produced, "test-rsa-emulator.log"

FYI, unittests.py reads the configuration file to determine the port
number to use to communicate with the emulator.

One can execute, the \"cf push\" and execute unittest.py sequentially
from the same Git Shell. Just use the URL from the \"cf push\" output.

> \$ cf push rsa-wip
>
> . . .
>
> instances: 1/1
>
> usage: 256M x 1 instances
>
> urls: rsa-wip.apps1-fm-int.icloud.intel.com
>
> last uploaded: Thu Mar 3 19:25:24 UTC 2016
>
> \$ python unittests.py Redfish "rsa-wip.apps1-fm-int-icloud.intel.com"

# Code Overview

The emulator uses Flask-RESTful, which allows easier RESTful code than
Flask.

## Flask

With the Flask package, the code associates each resource path and method are placed in separate Python methods.
This splits up the resource over multiple methods.
There's no encapsulation.
Below is an example of the code.

	```
	app = Flask(__name__)
	@app.route('/tasks', methods=['GET'])
	def get_all_tasks():
	return task_db.fetch_all_tasks()

	@app.route('/tasks', methods=['POST'])
	def create_task():
	task_string = request.form['task']
	task_db.create_task(task_string)
	return task_string

	@app.route('tasks/<int:id>', methods=['GET'])
	def get_task(id)
	return task_db.fetch_task(id)
	```

## Flask-RESTful

With the Flask-RESTful package, routes map directly into objects and the class's
methods are the same as their HTTP counterparts (e.g. get, post, etc.).
In other words, the resource and its methods are implemented as classes.
Below is an example of the code generated with does the same as above.

	```
	app = Flask(__name__)
	api = restful.Api(app)
	api.add_resource(Users, '/tasks')
	api.add_resource(User, '/tasks/<int:id>')

	class Users(restful.Resource):

		def get(self):
		return task_db.fetch_all_tasks()

		def post(self):
		task_string = request.form['task']
		task_db.create_task(task_string)
		return task_string

	class User(restful.Resource):

		def get(self, id):
		return task_db.fetch_task(id)
	```

## Program Flow

The **emulator.py** file loads the configuration (emulator-config.json) and obtain the configuration parameters (e.g. SPEC, MODE, TRAYS).
The code handles the non-Redfish paths (e.g. / and /redfish/v1/reset).
The code for 'RedfishAPI' class is defined as a Flask-RESTful class.

Control passes to the Resource Manager (**resource_manager.py**).
The resource manager loads the static resources from ./api_emulator/redfish/static.

The resource manager then attaches the dynamic resources to the hierarchy. 

Finally, the Emulator waits to receive HTTP requests.
As requests are received, additional requests are made to the resource manager support the request.

# Modifying the Emulator Source 

## Change the base URI

The base URI is defined in the field REST_BASE in emulator.py.
The rest of the emulator references this base URI.

	REST_BASE = '/redfish/v1/'

## Add Static resources

Static resources are added recursively from the Service Root. The process for adding a resource which is subordinated to the Service Root is:

1.  Copy the mockup folder tree for the resource into the ./api_emulator/redfish/static folder.
2.  In the *\_\_init\_\_* function of **resource_manager.py**, add a line to load the static resource tree. This line will recursive load the index.json files from the specified folder. 

	```
	self.AccountService = load_static('AccountService', 'redfish', mode, rest_base, self.resource_dictionary)
	```
3.  In the *configuration* function of **resource_manager.y**, add a line to the declaration in the definition of the variable, *config*.

	```
    @property
    def configuration(self):
        config = {
            . . .
            'AccountService': {'@odata.id': self.rest_base + 'AccountService'},
            . . .
        }
        return config
	```

## Add Dynamic resource

1. Implement the dynamic resource (see ...)
2. In the *\_\_init\_\_* function of **resource_manager.py**,
	1. If there is a *load_static* call for the same resource, then remove it.
	2. Add a call to *api.add_resource*.
	3. If the subordinate resource is collection resource, add a call to *api.add_resource* for the collection resource and one for the member resources.
	4. If a collection resource needs to start non-empty, create a member by adding a call to *Create<resource>.put* with the name of the member.

	```
	\# self.Chassis = load_static('Chassis', 'redfish', rest_base, self.resource_dictionary)
	g.api.add_resource(ChassisCollectionAPI, '/redfish/v1/Chassis/')
	g.api.add_resource(ChassisAPI, '/redfish/v1/Chassis/<string:ident>')
	config = CreateChassis()
	out = config.put('Test2')
	```
###Implement dynamic resources

Creating dynamic resources requires a little Python programming. The process is slightly difference for singleton resources versus collection resources.

- For singleton resources
	1. Write two Python files: a template file and an API file
	2. Edit the appropriate Python file to add the API to the RESTful interface
- For collection resources
	1. Write a Python file: an API file
	2. Edit the appropriate Python file to add the API to the RESTful interface

Since the collections resource are simple and similar, their template is incorporated in the code in the API file.
The API file includes HTTP function definitions for the singleton and its associated collection (if the singleton is a member of a collection).

The repository has tools to auto-generate these files from a mockup file. See the README.md.

The following sections describe the Template and API files, using chassis collection
resource and chassis resource as examples.

### Write a Template file

The dynamic resource constructor loads a template which instantiates the
resource and its properties. The template file has two sections, a
template declaration and a single function.

The template declaration is derived from the mockup file. The template
declaration is defined as a Python dictionary.
The template declaration contains substitution fields.
In the following example, the fields are {rb} and {id} fields will be replaced by the root_base and the instance ID, respectively.

	_TEMPLATE = \
		{
			"@odata.context": "{rb}\$metadata#Chassis.Chassis",
			"@odata.id": "{rb}Chassis/{id}",
			"@odata.type": "#Chassis.1.0.0.Chassis",
			"Id": "{id},
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
				"Status": "Enabled",
				"Health": "OK"
			},
			"Links": {
				"ComputerSystems": [{ "@odata.id": \"{rb}Systems/" }],
				"ManagedBy": [{ "@odata.id": "{rb}Managers/1" }],
				"ThermalMetrics\": { "@odata.id": "{rb}Chassis/{id}/ThermalMetrics" },
				"PowerMetrics": { "@odata.id": "{rb}Chassis/{id}/PowerMetrics" },
				"MiscMetrics": { "@odata.id": "{rb}Chassis/{id}/MiscMetrics" },
		}


The second section of the template file is the function definition. The
function is called to instantiate an instance of the resource.

First, the function makes a copy of the template declaration (using deepcopy).

The function replaces the substitution fields. 
The *rest_base* variable is set by the system to "redfish/v1". 
The *ident* variable is decoded from the id of the member.

The function sets the value of the SerialNumber property. In the example, the
default SerialNumber is overwritten by a randomly create 10 character string.

	def get_chassis_template(rest_base, ident):

	c = copy.deepcopy(_TEMPLATE)

	c['@odata.context'] = c['@odata.context'].format(rb=rest_base)
	c['@odata.id'] = c['@odata.id'].format(rb=rest_base, id=ident)
	c['Id'] = c['Id'].format(id=ident)
	c['Thermal']['@odata.id'] = c['Thermal']['@odata.id'].format(rb=rest_base, id=ident)
	c['Power']['@odata.id'] = c['Power']['@odata.id'].format(rb=rest_base, id=ident)
	c['Links']['ManagedBy'][0]['@odata.id'] =c['Links']['ManagedBy'][0]['@odata.id'].format(rb=rest_base, id=ident)
	c['Links']['ComputerSystems'][0]['@odata.id']=c['Links']['ComputerSystems'][0]['@odata.id'].format(rb=rest_base)

	c['SerialNumber'] = strgen.StringGenerator('[A-Z]{3}[0-9]{10}').render()

	return c

### Write an API file.

The API file specifies the behavior for each HTTP request.
The code below is pretty generic and can be copied for other resources.

The file includes three functions:
	- Behavior for HTTP requests for the singleton resource
	- Behavior for HTTP requests for the collection resource (if applicable)
	- Create of a singleton resource

For the examples below, the EgResource, and EgSubResouce classes are used. EgResource is a member of the EgResourceCollection and EgSubResource is a member of the EgSubResourceCollection.  The Python files are included in the repository.

The EgResource API class stores the members in a member[] list.

	members = {}

The function for the singleton resource implements the behavior for the HTTP request (GET, POST, PATCH and DELETE) are defined.
The example resource, EgResource, has subordinate a resource, EgSubResource. So the behavior of POST'ing a new EgResource, is that the subordinate resources should automatically be instantiated.
In the post() function, calls are made to api.add_resource to attached the subordinate resource, EgSubResource.
The collection resource for the EgSubResource will have no members.
The call to CreateEgSubResource will create a member.

	class EgResourceAPI(Resource):

    	def __init__(self, **kwargs):
        	try:
            	global wildcards
            	wildcards = kwargs
        	except Exception:
            	traceback.print_exc()

    	\# HTTP GET
    	def get(self, ident):
         	try:
            	resp = 404
            	if ident in members:
                	resp = members[ident], 200
        	except Exception:
            	traceback.print_exc()
            	resp = INTERNAL_ERROR
        	return resp

    	\# HTTP PUT
    	def put(self, ident):
        	return 'PUT is not a supported command for EgResourceAPI', 405

    	\# HTTP POST
    	def post(self, ident):
        logging.info('EgResourceAPI POST called')
        	try:
            	global config
            	config=get_EgResource_instance({'rb': g.rest_base, 'eg_id': ident})
            	members.append(config)
            	global done
            	if (done == 'false'):
                	collectionpath = g.rest_base + "EgResources/" + ident + "/EgSubResources"
                	logging.info('collectionpath = ' + collectionpath)
                	g.api.add_resource(EgSubResourceCollectionAPI, collectionpath, resource_class_kwargs={'path': collectionpath} )
                	singletonpath = collectionpath + "/<string:ident>"
                	logging.info('singletonpath = ' + singletonpath)
                	g.api.add_resource(EgSubResourceAPI, singletonpath,  resource_class_kwargs={'rb': g.rest_base, 'eg_id': ident} )
                	done = 'true'
            	cfg = CreateEgResource()
            	out = cfg.put(ident)
            	resp = config, 200
        	except Exception:
            	traceback.print_exc()
            	resp = INTERNAL_ERROR
        	return resp

    	\# HTTP PATCH
    	def patch(self, ident):

        	raw_dict = request.get_json(force=True)
        	try:
            	for key, value in raw_dict.items():
                	members[ident][key] = value
            	resp = members[ident], 200
        	except Exception:
            	traceback.print_exc()
            	resp = INTERNAL_ERROR
        	return resp

    	\# HTTP DELETE
    	def delete(self, ident):
        	try:
             	resp = 404
            	if ident in members:
                	del(members[ident])
                	resp = 200
        	except Exception:
            	traceback.print_exc()
            	resp = INTERNAL_ERROR
        	return resp

The code below is for the EgResourceCollectionAPI class.
The behavior for GET and POST are defined and PUT and DELETE return errors.
The post function adds the ChassisAPI to the interface.

	class ChassisCollectionAPI(Resource):

		def \_\_init\_\_(self):
        	self.rb = g.rest_base
        	self.config = {
            	'@odata.context': self.rb + '$metadata#EgResourceCollection',
            	'@odata.id': self.rb + 'EgResources',
            	'@odata.type': '#EgResourceCollection.EgResourceCollection',
            	'Name': 'EgResource Collection',
            	'Links': {}
        	}
        	self.config['Links']['Members@odata.count'] = len(members)
        	self.config['Links']['Members'] = [{'@odata.id':x['@odata.id']} for x in list(members.values())]


		\# HTTP GET
		def get(self):
			try:
				resp = self.config, 200
			except Exception:
				traceback.print_exc()
				resp = INTERNAL_ERROR
			return resp

    	def verify(self, config):
        	\# TODO: Implement a method to verify that the POST body is valid
        	return True,{}

    	\# HTTP POST
    	def post(self):
            try:
            	config = request.get_json(force=True)
            	ok, msg = self.verify(config)
            	if ok:
                	members[config['Id']] = config
                	resp = config, 201
            	else:
                	resp = msg, 400
        	except Exception:
            	traceback.print_exc()
            	resp = INTERNAL_ERROR
        	return resp

		\# HTTP PUT
		def put(self,ch_id):
			return 'PUT is not a valid command', 202

		\# HTTP DELETE
		def delete(self,ch_id):
			return 'DELETE is not a valid command', 202


### Add the resource to interface

With the API defined, the API needs to be attached to the Redfish hiearchy.
When the API should be available in the model will determine where the api.add_resource is placed.

1. If the resource is present when the Redfish starts, then the call to api.add_resource can be placed in resource_manager.py. This done for resources which are subordinated to Service Root.
2. If the resource is subordinate to another resource, the the call to api.add_resource should be placed in the POST behavior of the parent resource. For example, *./chassis/{id}/Thermal* should not be present, until *./Chassis/{id}* has been created.
3. If the resource is a member of a collection, then the call to api.add_resource should be placed in the Create function.

# Emulator Source Tree

This directory contain source for the root of the source tree. The following subdirectories are described in greater detail below:

-   root
-   api_emulator
-   api_emulator/redfish (Redfish resources)

## Root directory

The following table describes the files in the root directory

| Files | Description      |
| :---         | :---       |
| emulator.py  | The main Python file for the emulator. |
| emulator-conf.json | Configuration properties for the emulator |
| install.py | Installs the dependent python packages which are present in Dependencies folder. |
| requirements.txt | Lists the python packages which need to be installed\. The Cloud Foundry will install these packages as part of processing the \'cf push\ command. |
| ProcFile | The file executed by the Cloud Foundry at the end of the \'cf push\' |
| unittests.py | Tests the emulator locally. |
| unittests.data | Used by unittest.py for testing purposes |


## api_emulator directory

The following table describes the files in the api-emulator directory.

| Files | Description      |
| :---         | :---       |
| emulator_resource.py  | Loads the resources that become the resoure pool. Components of the tray are defined |
| exception.py | Exceptions thrown by the emulator |
| resource_manager.py | Attaches the resources that are emulated.  Attaches both the static and dynamic resources. |
| static_loader.py | Recursive loads static resources from the ./redfish/static directory. |
| utils.py | Supporting functions |

## api_emulator/redfish directory

The following table describes the folders in the redfish directory.

| Foldrs | Description      |
| :---         | :---       |
| ./  | Contains the implementatons for each dynamic resource's API |
| ./static | Contains the mockups for the statis resources |
| ./templates | Contains the templates for each dynamic resource. |
  
# ANNEX A (informative) Change log

| Version | Date       | Description |
| :---    | :---       | :---        |
| 1.0.0   | 2020-03-27 | Initial release. Converted from Word document. |
