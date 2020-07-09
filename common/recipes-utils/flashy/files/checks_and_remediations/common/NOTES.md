## TODO

* Restart services (need to clarify systemd and healthd with team)
* Reboot blocker, PS1 + `wall` alerts
* Check enough memory is available (this should be checked by external agent)
* Unmount devices that might be affected by upgrade process
* Check SPI chips are healthy
* Lock
    - check no other upgrading process
    - reboot block
    - shutdown block
