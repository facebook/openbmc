require recipes-core/images/ventura2-image.bb

IMAGE_CLASSES:append = " image_types_phosphor_aspeed_g7"

IMAGE_INSTALL:remove = " ncsi-strength"
