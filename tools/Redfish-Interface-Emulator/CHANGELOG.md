# Change Log

## [1.1.8] - 2022-07-15
- Various fixes to handling of composition requests

## [1.1.7] - 2022-01-10
- Added requirements for Python 3.9

## [1.1.6] - 2021-08-06
- Added support for /redfish/v1/odata
- Fixed usage of /redfish/v1/$metadata

## [1.1.5] - 2021-06-18
- Added Dockerfile to build Docker containers

## [1.1.4] - 2021-04-30
- Added support for /redfish

## [1.1.3] - 2021-04-16
- Corrected `RedfishVersion` property in service root
- Fixes to usage of static mockups
- Corrected `@odata.type` property in many resources
- Added support for emulating sessions and session service

## [1.1.2] - 2019-08-23
- Fixed CapacityBytes property to always be an integer

## [1.1.1] - 2019-08-09
- Fixed Status objects
- Fixed data type used for ServiceEnabled

## [1.1.0] - 2019-06-21
- Fixed many Service Root properties by moving them out of Links and to the root level

## [1.0.9] - 2019-05-10
- Corrected "Members@odata.count" property

## [1.0.8] - 2019-03-26
- Removed emulator_ssl.py
- Clarified how to use HTTPS
- Fixed properties presented in the ChassisCollection resource

## [1.0.7] - 2019-02-08
- Fixed exception handling with how messages are printed

## [1.0.6] - 2018-08-31
- Fixed how new resources are created via POST to allow for creation from templates as well as composed ComputerSystems

## [1.0.5] - 2018-07-16
- Cleanup of package dependencies
- Fixed problems with behavior of issuing POST to dynamic collections

## [1.0.4] - 2018-06-22
- Fixed handling of PATCH on dynamic resources

## [1.0.3] - 2018-06-01
- Made change so that both static and dynamic URLs do not have a trailing /

## [1.0.2] - 2018-05-04
- Fixed Systems collection to be called "Systems"

## [1.0.1] - 2018-03-02
- Fixed usage of the .values() method to match Python3 expectations

## [1.0.0] - 2018-02-16
- Added fixes for running with Python 3.5
- Added browser feature so a human can more easily navigate the service
- Added Composition Service
- Fixed support for running under Linux
- Added code generators for dynamic files
- Fixed inconsistencies for handling URIs with trailing / characters
- Support for setting of Link object paths
- Made Manager and ComputerSystem dynamic
- Implemented wildcards replacement for subordinate resources
- Fixes for Cloud Foundry execution

## [0.9.3] - 2017-05-21
- Change references from flask.ext.restful to flask_restful, since package is obsolete
- Add example files, eg_resource_api.py and eg_resource.py, showing how dynamic objects are coded.

## [0.9.2] - 2017-03-17
- Fixes to run with Python 3.5

## [0.9.0] - 2016-09-22
- Initial Release
