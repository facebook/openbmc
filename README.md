# OpenBMC

OpenBMC is an open software framework to build a complete Linux image for a Board Management Controller (BMC).

OpenBMC uses [Yocto](https://www.yoctoproject.org) as the underlying building framework.

## Contents

This repository includes 3 set of layers:

* **OpenBMC Common Layer** - Common packages and recipes can be used in different types of BMC.
* **BMC System-on-Chip (SoC) Layer** - SoC specific drivers and tools. This layer includes the bootloader (u-boot) and the Linux kernel. Both the bootloader and Linux kernel shall include the hardware drivers specific for the SoC.
* **Board Specific Layer** - Board specific drivers, configurations, and tools. This layer defines how to configure the image. It also defines what packages to be installed for an OpenBMC image for this board. Any board specific initialization and tools are also included in this layer.

## File structure

Yocto naming pattern is used in this repository. `meta-layer` is used to name a layer. And `recipe-abc` is used to name a recipe.

The recipes for OpenBMC common layer should be in `meta-openbmc/common`.

BMC SoC layer and board specific layer are grouped together based on the vendor/manufacturer name. For example, all Facebook boards specific code should be in `meta-openbmc/meta-facebook`. And `meta-openbmc/meta-aspeed` includes source code for Aspeed SoCs.

## How to build

* Step 0 - Set up the build environment based on the Yocto project [document](http://www.yoctoproject.org/docs/1.6.1/yocto-project-qs/yocto-project-qs.html).

* Step 1 - Clone Yocto repository.
```
$ git clone -b fido https://git.yoctoproject.org/git/poky
```

* Step 2 - Clone Openembedded and OpenBMC repositories, in the new created `poky` directory,
```
$ cd poky
$ git clone -b fido https://github.com/openembedded/meta-openembedded.git
$ git clone git@github.com:facebook/openbmc.git meta-openbmc
```

* Step 3 - Initialize a build directory. In `poky` directory,

```
$ export TEMPLATECONF=meta-openbmc/meta-facebook/meta-wedge/conf
$ source oe-init-build-env
```

After this step, you will be dropped into a build directory, `poky/build`.

* Step 4 - Start the build within the build directory, `poky/build`.

```
$ bitbake wedge-image
```

The build process automatically fetches all necessary packages and build the complete image. The final build results are in `poky/build/tmp/deploy/images/wedge`.

* u-boot.bin - This is the u-boot image for the board.
* uImage - This the Linux kernel for the board.
* wedge-image-wedge.cpio.lzma.u-boot - This is the rootfs for the board
* flash-wedge - This is the complete flash image including u-boot, kernel, and the rootfs.

# How can I contribute

If you have an application that can be used by different BMCs, you can contribute your application to the OpenBMC common layer.

If you are a BMC SoC vendor, you can contribute your SoC specific drivers to the BMC SoC layer.

If you are a board vendor, you can contribute your board specific configurations and tools to the Board specific layer. If the board uses a new BMC SoC that is not part of the BMC SoC layer, the SoC specific driver contribution to the BMC SoC layer is also required.
