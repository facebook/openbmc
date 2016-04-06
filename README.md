# OpenBMC

OpenBMC is an open software framework to build a complete Linux image for a Board Management Controller (BMC).

OpenBMC uses the [Yocto Project](https://www.yoctoproject.org) as the underlying building and distro generation framework.

## Contents

This repository includes 3 set of layers:

* **OpenBMC Common Layer** - Common packages and recipes can be used in different types of BMC.
* **BMC System-on-Chip (SoC) Layer** - SoC specific drivers and tools. This layer includes the bootloader (u-boot) and the Linux kernel. Both the bootloader and Linux kernel shall include the hardware drivers specific for the SoC.
* **Board Specific Layer** - Board specific drivers, configurations, and tools. This layer defines how to configure the image. It also defines what packages to be installed for an OpenBMC image for this board. Any board specific initialization and tools are also included in this layer.

## File structure

The Yocto naming pattern is used in this repository. A "`meta-layer`" is used to name a layer or a category of layers. And `recipe-abc` is used to name a recipe. The project will exist as a meta layer itself! Within the Yocto Project's distribution call this project `meta-openbmc`.

The recipes for OpenBMC common layer are found in `common`.

The BMC SoC layer and board specific layer are grouped together based on the vendor/manufacturer name. For example, all Facebook boards specific code should be in `meta-facebook`. Likewise, `meta-aspeed` includes source code for Aspeed SoCs.

## How to build

1. Set up the build environment based on the Yocto Project's [Quick Start Guide](http://www.yoctoproject.org/docs/1.6.1/yocto-project-qs/yocto-project-qs.html).

2. Clone Yocto repository:
```bash
$ git clone -b fido https://git.yoctoproject.org/git/poky
```

3. Clone OpenEmbedded and OpenBMC repositories, in the new created `poky` directory:
```bash
$ cd poky
$ git clone -b fido https://github.com/openembedded/meta-openembedded.git
$ git clone https://github.com/facebook/openbmc.git meta-openbmc
```
 Note that this project does not use Yocto release branch names.

4. Initialize a build directory. In the `poky` directory:
```bash
$ export TEMPLATECONF=meta-openbmc/meta-facebook/meta-wedge/conf
$ source oe-init-build-env
```
 Choose between `meta-wedge`, `meta-wedge100`, and `meta-yosemite`.
 After this step, you will be dropped into a build directory, `poky/build`.

5. Start the build within the build directory:
```bash
$ bitbake wedge-image
```

The build process automatically fetches all necessary packages and builds the complete image. The final build results are in `poky/build/tmp/deploy/images/wedge`. The root password will be `0penBmc`, you may change this in the local configuration.

#### Build Artifacts

* **u-boot.bin** - This is the u-boot image for the board.
* **uImage** - This the Linux kernel for the board.
* **wedge-image-wedge.cpio.lzma.u-boot** - This is the rootfs for the board
* **flash-wedge** - This is the complete flash image including u-boot, kernel, and the rootfs.

#### Yocto Configuration

It is recommended to setup a new Yocto distribution (a checkout of poky). The initialization script `oe-init-build-env` can read the included `TEMPLATECONF` and set up a local `build/conf/local.conf` along with the associated layer/build configuration.

If you have previously set up and built poky, you may change your local configuration:

When using the example `TEMPLATECONF` for Wedge, the `./build/conf/templateconf.cfg`:
```
meta-openbmc/meta-facebook/meta-wedge/conf
```

The layers config `./build/conf/bblayers.conf`, will contain:
```
BBPATH = "${TOPDIR}"
BBFILES ?= ""

BBLAYERS ?= " \
  /PREFIX/poky/meta \
  /PREFIX/poky/meta-yocto \
  /PREFIX/poky/meta-yocto-bsp \
  /PREFIX/poky/meta-openembedded/meta-oe \
  /PREFIX/poky/meta-openembedded/meta-networking \
  /PREFIX/poky/meta-openembedded/meta-python \
  /PREFIX/poky/meta-openbmc \
  /PREFIX/poky/meta-openbmc/meta-aspeed \
  /PREFIX/poky/meta-openbmc/meta-facebook/meta-wedge \
  "
BBLAYERS_NON_REMOVABLE ?= " \
  /PREFIX/poky/meta \
  /PREFIX/poky/meta-yocto \
  "
```

And finally the `./build/config/local.conf` will include important configuration options:
```
SOURCE_MIRROR_URL ?= "file://${TOPDIR}/../meta-openbmc/source_mirror/"
INHERIT += "own-mirrors"

BB_GENERATE_MIRROR_TARBALLS = "1"
BB_NO_NETWORK = "fb-only"

BB_NUMBER_THREADS ?= "${@oe.utils.cpu_count()}"
PARALLEL_MAKE ?= "-j ${@oe.utils.cpu_count()}"

MACHINE ??= "wedge"
DISTRO ?= "poky"
PACKAGE_CLASSES ?= "package_rpm"
SANITY_TESTED_DISTROS_append ?= " CentOS-6.3 \n "

USER_CLASSES ?= "buildstats image-mklibs image-prelink"
PATCHRESOLVE = "noop"
BB_DISKMON_DIRS = "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    ABORT,${TMPDIR},100M,1K \
    ABORT,${DL_DIR},100M,1K \
    ABORT,${SSTATE_DIR},100M,1K"

INHERIT += "extrausers"
EXTRA_USERS_PARAMS = " \
  usermod -s /bin/bash root; \
  usermod -p '\$1\$UGMqyqdG\$FZiylVFmRRfl9Z0Ue8G7e/' root; \
  "
OLDEST_KERNEL = "2.6.28"
INHERIT += "blacklist"
PNBLACKLIST[glibc] = "glibc 2.21 does not work with our kernel 2.6.28"
```

# How can I contribute

If you have an application that can be used by different BMCs, you can contribute your application to the OpenBMC common layer.

If you are a BMC SoC vendor, you can contribute your SoC specific drivers to the BMC SoC layer.

If you are a board vendor, you can contribute your board specific configurations and tools to the Board specific layer. If the board uses a new BMC SoC that is not part of the BMC SoC layer, the SoC specific driver contribution to the BMC SoC layer is also required.
