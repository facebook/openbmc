#!/bin/sh

echo "*****Running platform-svc HotPlugDetectionMechanism unit tests**********"
/usr/bin/test-platform-svc-hotplugdetectionmechanism

echo "*****Running platform-svc FRU unit tests********************************"
/usr/bin/test-platform-svc-fru

echo "*****Running platform-svc PlatformObjectTree unit tests*****************"
/usr/bin/test-platform-svc-platform-object-tree
