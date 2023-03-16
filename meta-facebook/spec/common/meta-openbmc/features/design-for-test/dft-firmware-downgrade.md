Firmware downgrade to a previous version of the BMC FW must not brick the
system.  This means the ability to downgrade the BMC to a previous version
must be supported.  Other firmware support (e.g. CPLD, BIC, etc.) must
support recovery via the BMC firmware update processes (either PLDM, Redfish
or via debug console utility (e.g. fw-util).  Also, this means if the platform
supports upgrade of the golden BMC image (e.g. ROM), it shall allow recovery
of the golden image as well to a known good state.
