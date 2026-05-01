Sensor names shall follow Meta's Sensor naming convention, satisfying the following RE pattern:
r"(?P<sensor_key>([A-Z0-9_]+_)+(TEMP_C|PWR_W|CURR_A|VOLT_V|SPEED_RPM|AIRFLOW_CFM|PWM_PCT|ENERGY_J|UTIL_PCT|PRESSURE_PA))$"
Sensor names shall not contain vendor names or vendor part identifiers. Slot, channel, or instance indices required to disambiguate sensors on multi-slot platforms (e.g., Yosemite v4) shall be encoded as numeric tokens in the prefix, such as `SLOT0_HSC_PWR_W`.
