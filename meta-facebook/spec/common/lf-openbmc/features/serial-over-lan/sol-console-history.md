The BMC must allow a user to access the history of the a Server/Slot's serial console
activity e.g to debug an issue that prevented Host boot progress.

This is sufficient to be available as log files in the BMC `/var/log/obmc-console-<host-identifier>.log`
host-identifier is host1-hostN for multi-host servers and host0 for single host servers.
