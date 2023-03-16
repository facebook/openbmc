The interface between the BMC and BIC shall use I3C as the primary hardware
communications path.  BIC to BIC communications shall use I3C as the interface
as well unless I2C is the only option.  BIC to external ASICs are to use I3C as
primary unless the ASIC does not support I3C in which case I2C should be used.
