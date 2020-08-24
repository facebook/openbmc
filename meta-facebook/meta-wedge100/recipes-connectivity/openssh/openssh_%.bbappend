# Read SSH keys from the persistent store. The init script on the SysV
# variant does this by creating symlinks at startup time. For systemd,
# it's easier to modify the configuration file to the right
# location. This will also make sshdgenkeys create the keys here on
# new devices.
do_configure_append() {
    # I hate myself for calling this "configuration"
    sed -i 's:.*HostKey.*\(/etc\):HostKey /mnt/data\1:' sshd_config
    sed -i '/ssh_host_key/d' sshd_config
}
