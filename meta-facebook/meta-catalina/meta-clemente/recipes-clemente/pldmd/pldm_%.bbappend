# - Disable fallback terminus name to prevent sensors
#   from being created when there is no correpoinding
#   Auxiliary Name PDRs available.
# - Enable fw update pkg inotify to monitor the PLDM
#   firmare package uploaded to /tmp/pldm_images.
EXTRA_OEMESON:append = " \
    -Denable-fallback-terminus-name=disabled \
    -Dfw-update-pkg-inotify=enabled \
"
