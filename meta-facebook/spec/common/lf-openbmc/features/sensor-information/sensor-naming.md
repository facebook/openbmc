Sensor names shall follow Meta's Sensor naming convention, satisfying the following RE pattern:
r"(?P<sensor_key>([A-Z0-9_]+_)+(TEMP_C|PWR_W|CURR_A|VOLT_V|SPEED_RPM|AIRFLOW_CFM|PWM_PCT|ENERGY_J|UTIL_PCT|PRESSURE_PA))$"
Sensor shall not have vendor name or identification in it's name
