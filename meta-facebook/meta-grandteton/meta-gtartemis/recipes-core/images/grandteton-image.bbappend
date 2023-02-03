# Copyright 2020-present Facebook. All Rights Reserved.

# Based on inspiration image and customized for gtartemis 
PROVIDES:remove = "inspirationpoint-image"
PROVIDES += "gtartemis-image"

# Include modules in rootfs
IMAGE_INSTALL += " \
   usbutils \
   usbip \
   "
