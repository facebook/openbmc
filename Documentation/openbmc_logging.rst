==========================
OpenBMC Logging Guidelines
==========================

Log messages are heavily used by developers and system administrators for
many purposes, such as trouble shooting, monitoring, profiling, statistics,
and etc.

Well-designed logs can speed up troubling shooting process greatly, while
bad logging could confuse people and introduce a lot of extra maintenance
overhead.

The document describes general logging guidelines when contributing to
Facebook OpenBMC.

Use Standard Logging API
========================

Use standard logging API instead of inventing your own logging libraries.
This is to ensure message format being consistent across platforms and
applications, and logging settings (log destination, rotation) can also
be easily controlled when needed.

For C/C++ applications/libraries:

  * please use "common/recipes-lib/log/" for logging.
    If the library does not meet your needs, please use standard syslog.

Simple Rules
------------

- Never use printf() in C/C++ libraries.

  Do NOT use printf() in library functions because the behavior varies
  depending on the calling context: "stdout" may be closed, re-directed
  to /dev/null or some other file/pipe/socket, which leads to undefined
  behavior.

Log Levels
==========

syslog defines 8 severity log levels, and we recommend using below 5 levels
in OpenBMC applications.

LOG_CRIT
--------

It usually denotes situations/actions that could end up in catastrophic
failures. Critical level logs are re-directed to "/mnt/data/logfile" and
they persist across BMC reboots.

Following events are good candidates for critical logs:

  * actions that affect host side components, such as power cycle chassis,
    reset Host/uServer, reset Switching ASIC, PIM, and etc.

  * conditions that affect system health, such as temperature exceeds limit.

  * persistent storage (such as eMMC) needs to be reformatted which may cause
    data loss.

LOG_ERR
-------

It indicates error conditions that prevent a certain component(s) from
working properly. Well defined error messages can be very helpful during
troubleshooting.

Below are a few examples:

  * "ipmid" failed to start because IPMI channel cannot be established.

  * FPGA firmware upgrade failed because the according SPI flash cannot
    be detected.

LOG_WARN
--------

It's used to report unexpected events. Such events don't always indicate
errors, but they could potentially become errors.
Warning messages should be investigated during troubleshooting.

For example:

  * configuration file does not exist, and default settings were applied.

  * gpio export failed because the pin was already exported.

LOG_INFO
--------

Such log messages should be purely informative and can be ignored during
normal operations. The most common ones are application life-cycle events,
such as "service started" or "service stopped".

For example:

  * "rackmond" started successfully at default baud rate 19200 bps.

  * PIM #1-8 were detected at bootup time.

LOG_DEBUG
---------

Debug level messages are designed for debugging purpose. Such messages
should NOT be printed during normal operations.

For example:

  * open /etc/service.conf and open() returned file descriptor 6.

  * it took 35 microseconds to execute foo() function.

Being Descriptive
=================

Log messages need to be descriptive and meaningful.

"Descriptive" might be a little subjective, but knowing the purpose and
audience of logging is the key to produce good logs. For example, people
should be able to answer below questions before adding/editing a logging
entry:

  * What is the purpose of the message? Is it an error condition, or
    purely informative, or just for debugging purpose?

  * Who will read/parse the message? Is it human or script?

Below are some simple tips/rules for better logging.

Simple Rules
------------

- Always include error code (errno) in error messages.

  Bad example::

    * "failed to open /etc/service.conf"

  Good example::

    * "failed to open /etc/service.conf: Permission Denied"

- Use proper prefix/suffix for hex/binary numbers.

  Bad example::

    * smbcpld register 33, value 80

  Good example::

    * smbcpld register 0x33, value 0x80

- Remember to specify units in logging messages.

  Bad example::

    * "volt sensor value: 100, temp sensor value: 32"

  Good example::

    * "volt sensor value: 100 mV, temp sensor value: 32 degree celsius"

- Use terms/conventions defined in hardware standard/specifications.

  Bad Example::

    * eMMC EXT_CSD[0xE7]: SECURITY_FEATURES: 0x55

  Good Example::

    * eMMC EXT_CSD[231], SEC_FEATURE_SUPPORT: 0x55

  In the above example, Decimal number 231 is preferred rather than 0xE7
  because decimal offsets are used in eMMC standard, thus people don't
  need to translate between hex/decimal when reading logs.
  Similarly, "SEC_FEATURE_SUPPORT" is preferred because people can easily
  lookup the keyword in eMMC standard.

Avoid Logging Too Much or Too Little
====================================

Excessive logging can seriously affect system performance, and it also becomes
hard to find useful information from thousands/millions lines of logs.

But meanwhile, too little log will make it extremely hard for trouble shooting.

It may take a few rounds to refine and adjust the amount of logs, but below are
some simple rules/tips:

Simple Rules
------------

- DO NOT log messages in kernel I/O functions.

  Do not dump messages in kernel I/O routines: return error code and let
  the callers decide how to handle the condition.

  For example, if an error message is dumped whenever I2C slaves NACK
  address, "i2cdetect" would trigger tens of "errors" but most "errors"
  are normal because the devices are not physically present.

- No errors or warnings during bootup.

  There shouldn't be any errors or warnings when booting up BMC on healthy
  hardware: if errors/warnings were logged at each reboot, something needs
  to be fixed.
