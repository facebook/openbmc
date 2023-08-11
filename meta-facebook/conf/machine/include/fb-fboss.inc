# Copyright (c) Meta Platforms, Inc. and affiliates.

#
# All the FBOSS platfroms should install libwatchdog-0.2 because they
# were moved out of kernel 4.1 (libwatchdog-0.1 is for kernel 4.1).
#
PREFERRED_VERSION_libwatchdog = "0.2"

#
# All the FBOSS BMC-Classis platforms should install wedge-eeprom-0.1.
# All the FBOSS BMC-Lite platforms should pick up wedge-eeprom-0.2 to
# support Meta EEPROM v4+, except fbdarwin (needs wedge-eeprom-0.1).
#
PREFERRED_VERSION_wedge-eeprom ?= "0.1"
PREFERRED_VERSION_libwedge-eeprom ?= "0.1"
