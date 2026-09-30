# - Disable fallback terminus name to prevent sensors
#   from being created when there is no correpoinding
#   Auxiliary Name PDRs available.
# - Enable fw update pkg inotify to monitor the PLDM
#   firmare package uploaded to /tmp/pldm_images.
# - Max out instance-id-expiration-interval and add a terminus
#   discovery retry to reduce false-positive Instance ID expiries and
#   give pldmd's own discovery layer a chance to recover a stuck
#   terminus without external intervention.
# - Raise dbus-timeout-value from upstream's 5s default to 60s.
EXTRA_OEMESON:append = " \
    -Denable-fallback-terminus-name=disabled \
    -Dfw-update-pkg-inotify=enabled \
    -Ddiscovery-fru-data-from-terminus=disabled \
    -Dinstance-id-expiration-interval=6 \
    -Dterminus-discovery-retry-count=2 \
    -Ddbus-timeout-value=60 \
"

do_install:append:openbmc-fb-lf() {
    rm -f ${D}/usr/share/pldm/host_eid
}
