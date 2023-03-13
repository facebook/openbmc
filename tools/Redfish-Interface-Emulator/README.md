Copyright 2016-2022 DMTF. All rights reserved.

# Redfish Interface Emulator

The Redfish Interface Emulator that can emulator a Redfish interface resources as static or dynamically.

Static emulator is accomplish by copy a Redfish mockup in a ./static directory.

Dynamic emulator is accomplished by creating the Python files.  The code for an example resource (EgResource) is available to expedite the creation of dynamic resources.  The example is for a collection/member construct, EgResources/{id}.

The Swordfish model has been emulate using this emulator.  The repository is available at [Swordfish API Emulator](https://github.com/SNIA/Swordfish-API-Emulator).  The repository provides a good example of the Python files for dynamic resources.

The program has been verified on 3.5.2 and 3.9.1. Use the packet sets to install the correct packages.

The emulator is structure so it can be ran locally, in a   Cloud Foundry, or as a Docker file.

## Execute Locally

When executing the Emulator locally, the Redfish service appears at port 5000, by default.  The Redfish client uses the following URL to access the emulator Redfish server - http://localhost:5000.  To run a second instance must a different 'port'. The port number can be changed via the command line parameters or the config.json file.

### Package Installation

The required python packages are listed in the files in the **./packageSets** directory.

The 'pip' command can be used to install the environment.

	pip install -r [packageSet]

If you are able to execute this emulator on later versions of Python, please issue the 'pip freeze' command.  Then place the output into a Github issue. We can add the package set to this repository.

### Invocation

Edit the emulator-config.json file and set **"MODE": "Local"**, then start the emulator.

	python emulator.py

## Execute on a Cloud Foundry

The cloud foundry method has be successful within a company internal cloud foundry service. It has not be attempted on a public cloud foundry service.

### Package Installation

The required python packages for a Cloud Foundry environment are listed in the file **./requirements.txt**.  The file lists the Python package, without the revision.
The packages will be installed automatically during invocation.

The cloud foundry makes use of the following files: requirements.txt, runtime.txt, and Profile. So they should exists in the same directory as emulator.py.

### Invocation

Edit the emulator-config.json file and set **"MODE": "Cloud"**, then push the emulator to the foundry.

	cf push [foundry-app-name]

The **foundry-app-name** determines the URL for the Redfish service.

## Execute via Docker

### Installation

Use one of these actions to pull or build the container:

* Pull the container from Docker Hub:

    ```bash
    docker pull dmtf/redfish-interface-emulator:latest
    ```
* Build a container from local source:

    ```bash
    docker build -t dmtf/redfish-interface-emulator:latest.
    ```
* Build a container from GitHub:

    ```bash
    docker build -t dmtf/redfish-interface-emulator:latest https://github.com/DMTF/Redfish-Interface-Emulator.git
    ```

### Invocation

This command runs the container with the built-in mockup:

```bash
docker run --rm dmtf/redfish-interface-emulator:latest
```

## Configuring the Emulator
The behavior of the emulator can be control via command line flags or property values in emulator-config.json.

### Emulator app flags

The emulator is invoked with the following command:

    python emulator.py [-h] [-v] [-port PORT] [-debug]
    -h -- help (gives syntax and usage) 
    -v -- verbose
    -port -- specifies the port number to use
    -debug -- enables debugging (needed to debug flask calls)

### Format of emulator-config.json

The emulator reads the emulator-config.json file to configure its behavior.
    
    {
        "MODE": "Local"
        "HTTPS": "Disable",
        "SPEC": "Redfish",
        "STATIC": "Enable",
        "TRAYS": [
            "./Resources/Systems/1/index.json"
        ]
        "POPULATE": "Emulator"
    }


* The MODE property specifies port to use. If the value is 'Cloud', the port is assigned by the cloud foundry. If the value is ‘Local’, the port is assigned the value of the port parameter is the command line or 5000, by default. (e.g. localhost:5000/redfish/v1)
* The HTTPS property specifies whether HTTP or HTTPS will used by the client
* The STATIC property specifies whether the emulator runs with the mockups in ./api_emulator/redfish/static
* The SPEC property specifies whether the computer system is represented as a Redfish ComputerSystem (SPEC = “Redfish”) or another schema.
* The TRAYS property specifies the path to the resources that will make up the initial resource pools. These simple resource pools are depleted as computer systems are composed. Multiple trays can be specified.
* The POPULATE property specifies the path to the file used by the INFRAGEN module to populate the Redfish Interface backend. If the file does not exist or if POPULATE is not defined, the emulator will start empty.

Three sample configuration files are provided:

* emulator-config_static.json (default) - start emulator with the static mockup
* emulator-dontpopulate.json - start emulator with no chassis or systems
* emulator-populate.json - start emulator and populate with **infragen**

### HTTPS
The emulator supports HTTP and HTTPS connections.  HTTPS is enabled by setting the HTTPS property in emulator-config.json.

    {
        "HTTPS": "Enable",
 		...
    }

When HTTPS is enabled, the emulator looks for the files: **server.crt** and **server.key** in the same directory as emulator.py.  The certificate file and key file can be self-signed or obtained from a certificate authority.

## Static emulation of a mockup

The emulator can be used to support static mockups. In a static mock, only HTTP GETs will work.  Other Redfish simulators support static mockups.

The static mockup is found in the directory ./api_emulator/redfish/static.  The emulator comes with a sample Redfish mockup already in the directory.  This can be replaced with any mockup folder hierarchy. The Redfish Forum has posted several of their mockups in [DSP2043](https://www.dmtf.org/sites/default/files/DSP2043_1.2.0.zip).

Note: If the new mockup has additional resources in the ServiceRoot, then modifications need to be made in static_resource_emulator.py to adds these new resources.

## Dynamic emulation
The emulator was designed to support dynamic resources.  This requires that Python code exists for each dynamic resource. Resources which are static and dynamic can co-exist in an emulator. This means one can straddle static vs dynamic emulation, with some resources static while others are dynamic.

Dynamic resource implementations which have been implemented are provided in the Appendix.

The following outlines the overall process. More complete documentation is in a Word document in the ./doc directory. To expedite the creation of the API-file and template file, the code generators for both files are described.

### Manually creating a dynamic resource
A dynamic resource is made by creating a template-file and an API-file for the resource.

* The template-file contain the properties of the resource.  The file is used when a new instance of the resource is created.
* The API-file contain the behavior of the resource for each HTTP command.  If there is an associated collection resource, the file also contains the behavior of the collection resource.

Once the files are created, they are placed in the emulator directory structure.

* The template-file is placed in the directory ./api\_emulator/Redfish/template
* The API-file is placed in the directory ./api\_emulator/Redfish
* If the resource in the Service Root, the the emulator.py file needs to be edited.
	* If the dynamic resource replaces a static resource, then replace the line which loads the static mockup with the line to add to dynamic resource API.

### Auto-generate the API file
To generate a API file, execute the following command

	codegen_api [mockup] [outputdir]

Where

* \[mockup\] is the name of the singleton resource
* \[outputdir\] is the directory for the API file
* The command uses the index.json file in the current work directory as input

The generated code supports the HTTP GET, PATCH, POST and DELETE commands

If the resource has subordinate resources that need to be instantiated when this resource is instantiated, that code will need to be manually added.

### Auto-generate the template file
To generate a template file, execute the following command

	codegen_template [mockup] [outputdir]

Where

* \[mockup\] is the name of the singleton resource
* \[outputdir\] is the directory for the API file
* The command uses the index.json file in the current work directory as input   
   
 (TODO - add the filename as a command line parameter)

The codegen_template source file contains a dictionary with the names of Redfish collections and their corresponding wildcard.  This dictionary needs to be manually updated to the keep in sync with Redfish modeling. 

## Auto-populating the dynamic emulator

Once a resource is made dynamic, the emulator can either start up with no members in its collections or some initial set of members.

To populate the Redfish model, set the POPULATE property in emulator-config.json.

    {
        "POPULATE": "Emulator",
		. . .
    }

Once the emulator has started, it will read the file **./infragen/populate-config.json**.  This file contains a JSON structure which specifies resources to populate.  The following example specifies that 5 Chassis be instantiated and linked to 5 Systems.

```
{
  "POPULATE": {
    "Chassis": [
      {
        "Name": "Compute Chassis",
        "Id": "Chassis-{0}",
        "Count": 5,
        "Links": {
          "ComputerSystems": [
            {
              "Name": "Compute System",
              "Id": "System-{0}",
              "Count": 1,
              "Processors": [
                {
                  "Id": "CPU{0}",
                  "TotalCores": 12,
                  "MaxSpeedMHz": 2400,
                  "Count": 2
                }
              ],
              "Memory": [
                {
                  "Id": "DRAM{0}",
                  "CapacityMiB": 16384,
                  "MemoryType": "DRAM",
                  "Count": 4
                },
                {
                  "Id": "NVRAM{0}",
                  "CapacityMiB": 65536,
                  "MemoryType": "NVDIMM_N",
                  "Count": 4
                }
              ],
              "SimpleStorage": [
                {
                  "Id": "SAS-CTRL{0}",
                  "Count": 1,
                  "Devices": {
                    "CapacityBytes": 549755813888,
                    "Count": 1
                  }
                }
              ],
              "EthernetInterfaces": [
                {
                  "Id": "NIC-{0}",
                  "SpeedMbps": 10000,
                  "Count": 2
                }
              ]
            }
          ]
        }
      },
```

### INFRAGEN module

The INFRAGEN module is used to populate the Redfish Interface with members (Chassis, Systems, Resource Blocks, Resources Zones, Processors, Memory, etc).

This module is execute by emulator.py once the emulator is up and reads the populate-config.json file.

The tool generate_template.py can be used to help a user with the creation of JSON template according to a specific Redfish schema by guiding the user through the schema and asking for input values. This module runs independently of the populate function and from the emulator itself.

## Testing the (dynamic) Emulator

The following is the general command for running unit test.

    python unittests.py <SPEC> "<PATH>"

Where

* <SPEC> should correspond to the value in emulator-config.json
* <PATH> is the path to use for testing and should be enclosed in double-quotes
* Example: python unittests.py Redfish "localhost:5000"

Once the command completes, inspect the log file which is produced, "test-rsa-emulator.log".

The command to test the emulator can executed against the emulator running locally or hosted in the cloud.

## Composition GUI

Generally, Postman is used to interact with the Redfish interface. However, a web GUI is available for testing the composition service, including the ability to compose and delete systems.

* Compose
	* Navigate to /CompositionService/ResourceZones/\<ZONE\>/, a check box appears next to each Resource Block
	* Select the Resource Blocks to use to compose a system
	* Press the button "compose" on the top of the webpage
* Delete
	* Navigate the browser to /Systems/\<SYSTEM\>/, if the system is of type "COMPOSED", a "delete" button appears
	* Select the delete button and the composed system will be deleted

Screenshots of the browser available in /doc/browser-screenshots.pdf

To use, point a brower to the URI **http://localhost:5000/browse.html**

## Release Process

Run the `release.sh` script to publish a new version.

```bash
sh release.sh <NewVersion>
```

Enter the release notes when prompted; an empty line signifies no more notes to add.

# Appendix - Available Dynamic Resources implementations

The emulator is made dynamic by added python code to emulate a resources RESTful behavior, call the API-file. 

Dynamic resources implementations can be found in the Redfish Interface Emulator repository and the SNIA API Emulator repository.

The repository also had the codegen_api code generator for creating an API-file for a resource with some default behaviors.  The code generator takes a mockup file as input.

## Redfish Interface Emulator

The Redfish Interface Emulator comes with a small set of API-files to demonstrate the code structure.  The repository also had the codegen_api code generator for creating an API-file for a resource with some default behaviors.  The code generator takes a mockup file as input.

These dynamic resources are available in the [Redfish Interface Emulator repository](https://github.com/dmtf/Redfish-Interface-Emulator) in the api_emulator/redfish directory.

| Resource          | API-file
| :---              | :---   
| ./CompositionService | CompositionService_api.py  
| ./EventService | event_service.py  
| ./SessionService |  
| ./SessionService/Sessions/{id} | sessions_api.py  
| ./Chassis/{id} | Chassis_api.py     
| ./Chassis/{id}/Thermal | thermal_api.py  
| ./Chassis/{id}/Power | power_api.py  
| ./Systems/{id} | ComputerSystem_api.py  
| ./Systems/{id}/Memory | memory.py  
| ./Systems/{id}/Processors/{id} | processor.py   
| ./Systems/{id}/SimpleStorage/{id} | simple_storage.py  
| ./Managers/{id} | Manager_api.py   


## SNIA API Emulator

The Swordfish API Emulator added the API-files to emulate the Swordfish schema.

These dynamic resources are available in the [SNIA API Emulator repository](https://github.com/SNIA/Swordfish-API-Emulator) in the api_emulator/redfish directory.

| API-file          | Resource
| :---              | :---                                
| . | serviceroot_api
| ./EventService/Subscriptions | Subscriptions.py
| ./Chassis/{id} | Chassis_api.py     
| ./Chassis/{id}/Memory | c_memory  
| ./Chassis/{id}/Drives/{id} | drives_api.py
| ./Chassis/{id}/MediaController | MediaControllers_api.py  
| ./Chassis/{id}/MediaControllers/{id}/Ports| mc_ports_api.py  
| ./Chassis/{id}/NetworkAdapters | networkadapaters_api.py
| ./Chassis/{id}/NetworkAdapters/{id}/NetworkDeviceFunctions/{id} | networkdevicefunctions_api.py  
| ./Chassis/{id}/NetworkAdapters/{id}/Ports/{id} | nwports_api\.py
| ./Systems/{id} | ComputerSystem_api.py   
| ./Systems/{id}/Storage/{id}/Volumes/{id} | volumes_api  
| ./Systems/{id}/Storage/{id}/StoragePools/{id} | storagepools_api.py  
| ./Systems/{id}/Memory/{id} | memory.py
| ./Systems/{id}/Memory/{id}/MemoryDomains/{id} | memory_domains_api.py
| ./Systems/{id}/Memory/{id}/MemoryDomains/{id}/MemoryChunks/{id} | md_chunks_api.py
| ./Fabrics/{id}/Connections/{id} | f_connections.py   
| ./Fabrics/{id}/EndpointGroups/{id} | f_endpointgroups.py
| ./Fabrics/{id}/Endpoints/{id} | f_endpoints.py
| ./Fabrics/{id}/Switches/{id} | f_switches.py 
| ./Fabrics/{id}/Switches/{id}/Ports/{id} | f_switch_ports_api.py
| ./Fabrics/{id}/Zones/{id} | f_zones_api.py
| ./Fabrics/{id}/FabricAdapters/{id} | fabricadapters.py
| ./Fabrics/{id}/FabricAdapters/{id}/Ports/{id} | fa_ports_api.py
| ./Storage | storage_api.py
| ./Storage/{id}/Controllers/{id} | storagecontrollers_api
| ./Storage/{id}/FileSystems | filesystems_api.py  
| ./Storage/{id}/StorageGroups  | storagegroups_api.py
| ./StorageSystems/{id} | storagesystems_api.py
| ./StorageServices/{id}/ClassesOfService | classofservice_api.py  
| ./StorageServices/{id}/DataProtectionLoSCapabilities | dataprotectionloscapabilities_api.py   
| ./StorageServices/{id}/DataSecurityLoSCapabilities | datasecurityloscapabilities_api.py   
| ./StorageServices/{id}/DataStorageLoSCapabilities| datastorageloscapabilities_api.py   
| ./StorageServices/{id}/EndpointGroups | EndpointGroups_api.py   
| ./StorageServices/{id}/Endpoints | Endpoints_api.py   
| ./StorageServices/{id}/IOConnectivityLoSCapabilities | IOConnectivityLOSCapabilities_api.py   
| ./StorageServices/{id}/IOPerformanceLoSCapabilities | IOPerformanceLOSCapabilities_api.py   

