The fan control algorithms should, by default, be controlled with a JSON-defined
algorithm acceptable by the system's thermal characterization team.  This JSON
definition should be able to be side-loaded with new algorithms (via SSH) and
optionally persisted to replace the algorithm contained in the base flash image.

Some systems have hardware sensors which require calibration based on other
environmental factors, such as a Voltage Regulator which gives inaccurate
readings depending on the temperature of the VR.  These calibrations should be
able to be applied to sensors.

There should be an airflow sensor calculated based on the fan speed and
hardware configuration.
