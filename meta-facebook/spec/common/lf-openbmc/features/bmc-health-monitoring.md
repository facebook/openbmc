The BMC must have a watchdog timer to ensure the BMC is still executing.  This
should be configured with the systemd watchdog support so that systemd is
responsible for petting the watchdog.

The overall health of the BMC should be monitored (CPU and memory usage).  When
the memory usage drops below a safe threshold, a `xyz.openbmc.Logging.Entry`
shall be created and the the BMC should be reset.  When the CPU usage of the
BMC is above a safe threshold for a period of time, a
`xyz.openbmc.Logging.Entry` shall be created.

There should be a method to disable automatic BMC reboots for debug purposes.
