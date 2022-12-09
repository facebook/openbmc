# Copyright 2020-present Facebook. All Rights Reserved.

# Based on grandteton image and customized for inspirationpoint 
PROVIDES:remove = "grandtenton-image"
PROVIDES += "inspirationpoint-image"

# Include modules in rootfs
IMAGE_INSTALL:remove = " \
   asd \
   asd-test \
   me-util \
   "

IMAGE_INSTALL += " \
   apml \
   "
