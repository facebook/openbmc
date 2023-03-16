If a WDT triggers while loading a new firmware image, the BMC shall
recover to the golden (e.g. ROM) image for the BMC so the BMC can
boot.  It is expected other firmware types will be updated by data
center tooling after the BMC recovery is complete.
