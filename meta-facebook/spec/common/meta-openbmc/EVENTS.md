# Meta OpenBMC Common Events

This document of common event logs we can expect. Do note that not all
these event logs might be applicable to all platforms.

# IPMI Events

# Healthd Events
Events which are not indicative of the health of the BMC but more to the health of the healthd itself
```
HEALTHD: Warning: Ignoring unsupported I2C Bus:<NUM>
HEALTHD configuration load failed
```

## I2C Monitoring
```
DEASSERT: I2C(<I2C_BUS>) Bus recovered. (I2C bus index base 0)
ASSERT: I2C(<I2C_BUS>) bus is locked (Master Lock or Slave Clock Stretch). Recovery error. (I2C bus index base 0)
ASSERT: I2C(<I2C_BUS>) bus is locked (Master Lock or Slave Clock Stretch). Recovery timed out. (I2C bus index base 0)
I2C(%d) bus had been locked (Master Lock or Slave Clock Stretch) and has been recoveried successfully. (I2C bus index base 0)
ASSERT: I2C(%d) Slave is dead (SDA keeps low). Bus recovery error. (I2C bus index base 0)
ASSERT: I2C(%d) Slave is dead (SDAs keep low). Bus recovery timed out. (I2C bus index base 0)
I2C(%d) Slave was dead. and bus has been recoveried successfully. (I2C bus index base 0)
ASSERT: I2C(%d) Undefined case. (I2C bus index base 0)
```

## BMC CPU Monitoring
```
Cannot get CPU statistics. Stop %s

```

## ME Monitoring
```
ASSERT: ME Status - Controller Unavailable on the <FRU_NAME>
DEASSERT: ME Status - Controller Unavailable on the <FRU_NAME>
ASSERT: ME Status - Controller Access Degraded or Unavailable on the %s, result: <HEX>, <HEX>
DEASSERT: ME Status - Controller Access Degraded or Unavailable on the <FRU_NAME>
```

## Verified Boot
```
ASSERT: Verified boot failure (<ERROR_TYPE>,<ERROR_CODE>)
Verified boot failure reason: <STR_ERROR>
DEASSERT: Verified boot failure (<ERROR_TYPE>,<ERROR_CODE>)
```


# Sensor Events

Threshold sensor events are of fixed format.
```
ASSERT: <THRESHOLD_TYPE> threshold - raised - FRU: <FRU_NUM>, num: 0x<SENSOR_NUM> curr_val: <FLOAT_VALUE> <UNITS>, thresh_val: <FLOAT_VALUE> <UNITS>, snr: <SENSOR_NAME>
ASSERT: <THRESHOLD_TYPE> threshold - settled - FRU: <FRU_NUM>, num: 0x<SENSOR_NUM> curr_val: <FLOAT_VALUE> <UNITS>, thresh_val: <FLOAT_VALUE> <UNITS>, snr: <SENSOR_NAME>
THRESHOLD_TYPE Holds one of the following values:
Upper Non Critical
Upper Critical
Upper Non Recoverable
Lower Non Critical
Lower Critical
Lower Non Recoverable
```
FRU_NUM, SENSOR_NUM, SENSOR_NAME are platform specified and will be documented in the Sensor spec. Look for the documented thresholds there.

