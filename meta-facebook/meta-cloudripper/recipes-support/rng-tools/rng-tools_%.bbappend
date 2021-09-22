PACKAGECONFIG:remove = "libjitterentropy"

#disable for now as rng-tools rely on deprecated systemd-udev-settle.service 
SYSTEMD_AUTO_ENABLE:rng-tools = "disable"
