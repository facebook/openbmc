
# linux 2.6.28 does not support devtmpfs, manually populate dev
USE_DEVFS = "0"

IMAGE_DEVICE_TABLES += "recipes-core/images/files/aspeed_device_table"