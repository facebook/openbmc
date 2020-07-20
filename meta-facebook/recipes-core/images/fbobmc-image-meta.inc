IMAGE_FEATURES[validitems] += "image-meta"

inherit ${@bb.utils.contains("IMAGE_FEATURES", "image-meta", \
  "fbobmc-image", "kernel_fitimage", d)}


