# Redfish API Emulator v1.0

This document has the following sections

-   Emulator Abilities

-   Emulator Usage to create Redfish computer systems

-   OVF Repository Administration

# Emulator Abilities

The emulator has the following capabilities:

-   Support HTTP and HTTPS

-   Ability to create a computer system

-   Ability to disable a Computer System

-   Ability to reset a Computer System

-   Ability to subscribe for an Event

-   Randomize strings within given format (e.g. serial numbers)

# RESTful Browser

Most browsers (e.g. Explorer, Chrome) will only allow you to perform a
GET.

To perform the other RESTful commands (POST, PUT, DELETE), you need to
use an add-on. There are two plug-ins for Chrome, both called Postman:

-   Chrome comes with a \"Postman -- REST Client\" application

-   The site getpostman.com has a \"Postman\" plug-in for Chrome

The later (from getpostman.com) is recommended. It auto-detects the
links in the JSON response messages which makes traversing the links
more efficient, since you don\'t need to type the URL.

The following emulator instance exposes the Redfish v1.0.1 interface

-   <http://redfish.apps1-fm-int.icloud.intel.com/redfish/v1>

# Emulator Usage

In the instructions below, \<URL\> should be replaced with location HTTP endpoint serviced by the emulator.

## Using HTTPS

The emulator needs to be started with the HTTPS option.

Use the credentials 'Administrator':'Password' for admin role or
'User':'Password' for ReadOnlyUser role.

## Create a Computer System resource

To create a generic computer system, issue the following command:

POST
http://\<URL\>/redfish/v1/Systems/?Action=CreateGenericComputerSystem

With the following body.

{

    \"Action\": \"CreateGenericComputerSystem\",

    \"NumberOfProcessors\": \"2\",

    \"TotalSystemMemoryGB\": \"4\",

    \"NumberOfNetworkPorts\": \"1\",

    \"Devices\": \[ { \"Name\": \"Drive 1\", \"Size\": \"120 GB\" } \],

    \"Boot\":

    {

        \"BootSourceOverrideEnabled\": \"True\",

        \"BootSourceOverrideTarget\": \"PXE\",

        \"UefiSourceOverrideEnabled\": \"...\"

    }

}

The HTTP response is the Computer System resource.

## Disable a System

To disable the computer system issue the following command:

POST http://\<URL\>/redfish/v1/Systems/1/?Action=ApplySettings

With the following body.

{

   "Status": { "State": "Disabled" }

}

The HTTP response is the Computer System resource.

## Reset a System

To disable the computer system issue the following command:

POST http://\<URL\>/redfish/v1/Systems/1/?Action=Reset

With the following body.

{

   "PowerState": \"Off\"

}

The HTTP response is the Computer System resource. The value of the
PowerState property is \"Off\" and will remain so for about 10 seconds.

Issue the following GET command and watch the value toggle back to
\"On\"

GET http://\<URL\>/redfish/v1/Systems/1

## Subscribing for events

To subscribe to events, issue the following command:

POST http://\<URL\>/redfish/v1/EventService/?Action=Subscribe

With the following body. Where:

-   dest_url is the URL for where the event is to be sent

-   Types property can contain a subset of the list: Alert,
    ResourceChanged, ResourceUpdated, ResourceDeleted, and StatusChange.

{

\"Destination\": \"(dest-url)\",

\"Types\": \[ \"StatusChange\" \],

\"Context\": \"SystemAdmin\"

}

## Receiving an event

Once you have subscribe for an event, to receive an event, perform the
following

1.  Start listening on URL from subscription request

2.  Make a POST to change a resource (see Reset a System and Disable a
    System)

## Reset the Emulator

To reset the emulator (i.e. remove all created computer systems) issue a
DELETE request to the following URL:

DELETE http://\<URL\>/redfish/v1/reset/
