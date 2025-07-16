require common/images/fb-openbmc-image.inc

# Install temporary firmware update utilities for POC phase.
IMAGE_INSTALL:append = " bios-fw-update"
IMAGE_INSTALL:append = " cpld-fw-handler"
IMAGE_INSTALL:append = " pldm-fw-update"
IMAGE_INSTALL:append = " pldm-package-re-wrapper"
IMAGE_INSTALL:append = " cxl-fw-update"
IMAGE_INSTALL:append = " addc"
IMAGE_INSTALL:append = " yaap"
IMAGE_INSTALL:append = " compute-software-id"
IMAGE_INSTALL:append = " fw-versions"
IMAGE_INSTALL:append = " pmic-error-injection"
# T214301589: Added as a temp fix for power sensor collection on T2 machines
IMAGE_INSTALL:append = " sensor-cache"
# T214925969: Temp measure to allow fans to be set to a particular PCT on boot
IMAGE_INSTALL:append = " fan-manual-mode"
# A service for asking for reboot assistance via SEL event
IMAGE_INSTALL:append = " managed-reboot"
# Added as a temp way to reset individual nics
IMAGE_INSTALL:append = " nic-reset"
# Added to determine when PLDM is missing sensors
IMAGE_INSTALL:append = " pldm-monitor"
# Added to ensure that nic temp sensors are brought back when they disapear
IMAGE_INSTALL:append = " nic-sensor-monitor"
# Add expect needed for NIC console development
IMAGE_INSTALL:append = " expect"
