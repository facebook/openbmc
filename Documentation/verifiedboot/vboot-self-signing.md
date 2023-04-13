# Verified boot Self Sign


## Vboot-sign script

There’s a help vboot-sign script in OpenBMC source code (
tools/verified-boot/signing/vboot-sign) can be used to



* Generate the vboot self sign manifests (keys, subordinate key-stores)
* Execute the self-sign after you build the image

```
[~/openbmc (helium)]$ tools/verified-boot/signing/vboot-sign --help
USAGE: tools/verified-boot/signing/vboot-sign [OPTIONS]
OPTIONS:
--help
    Print this message
-k|--keys KEYS_DIR
    Set Keys directory to KEYS_DIR, else the path of this script will be used
-b|--build BUILD_DIR
    Set the build directory to BUILD_DIR. Else the current working directory will be used
-s|--setup
    Generate the keys in KEYS_DIR (See above for default path)
-S|--HSM {compute|storage|network|training}
    Use FB HSM keyname ncipher/simple/obmc-<platform_type>-sub-0 key to sign
--force
    Force signing with self-signed, non-HSM keys on new images
```




### Prerequisite

1. Install jinja2 and pycrypto

The vboot self sign key generation process depends on two extra python modules:
jinja2 and pycrypto which need to be installed using pip.

On the dev-server, the default python3 is the production python which shall use
[PyPI
](https://www.internalfb.com/intern/wiki/Third-Party_for_Megarepo/Python_(PyPI)/)to
install the third part package. As we don’t want to create the self sign key in
production, so we don’t take the hassle of introducing JinJa2 and PyCrypto into
our fbcode python.

Instead, you can specify which python the vboot-sign script to use via -p option
i.e.  `-p /usr/bin/python3`

And you can use pip3.x to install the two packages for python3.x your devserver
centos installed; you may need to install python3-devel for install pycrypto.


```
sudo dnf install python3-devel

pip3.6 install --user jinja2
pip3.6 install --user pycrypto
```




2. Successfully build an openbmc image

The vboot-sign currently is using the uboot mkimage which is built during our
openbmc build.

So you need to at least have a successful openbmc build.


### Create the self signed keys (Just need one time)

The first time this action is taken: use vboot-sign to set up the self sign
environment (the signing artifacts).

The assumption is a script to save all generated self-signed data in the
~/workspaces/obmc-selfsign.  Any platform build directory will work.  As an
example, for the Grand Canyon build output, the expectation is it will be in the
build-fbgc directory.

```
# Create the vboot self-signing keys in ~/workspaces/obmc-selfsign
# on dev-server using /usr/bin/python3 provided by centos 8
[~/workspaces/openbmc/build-fbgc
 (helium)]$ ../tools/verified-boot/signing/vboot-sign -s \
   -p /usr/bin/python3 \
   -k ~/workspaces/obmc-selfsign
```

Keys are generated in the "workspaces/obmc-selfsign" directory under your home
directory.
```
# Sign the image
# run the script within the openbmc build folder,
# the script will automatically discovery the image to be signed
# in tmp/deploy/images/<platform>/flash-<platform>
# -i|--image can be used to specify the image to be signed
# Notice: specify --force to have the image signed with a self-signed key
# instead of an HSM managed production key.

[~/workspaces/openbmc/build-fbgc (helium)]$ ../tools/verified-boot/signing/vboot-sign \
   -p /usr/bin/python3 \
   -k ~/workspaces/obmc-selfsign \
   --force
```

sign the image with generated self sign keys
```
# Generate locked (lock & sign) image
# First, use the fit-sign utility to set the lock bit of the image.
[~/workspaces/openbmc/build-fbgc (helium)]$ ../tools/verified-boot/signing/fit-sign --lock tmp/deploy/images/grandcanyon/flash-grandcanyon flash-grandcanyon.lock
...

Wrote unsigned firmware: flash-grandcanyon.lock

# Then sign the lock bit set image
[~/workspaces/openbmc/build-fbgc (helium)]$ ../tools/verified-boot/signing/vboot-sign \
   -p /usr/bin/python3 \
   -k ~/workspaces/obmc-selfsign \
   --force \
   -i flash-grandcanyon.lock

...
Wrote signed firmware: ... /build-fbgc/flash-grandcanyon.lock.signed
```



### Verifying the signed image

The vboot-check script (which will be installed in OpenBMC image also), can be
used to check the signed image.


```
[~/workspaces/openbmc/build-fbgc (helium)]$ ../common/recipes-utils/vboot-utils/files/vboot-check tmp/deploy/images/grandcanyon/flash-grandcanyon.signed

Try loading image meta from image tmp/deploy/images/grandcanyon/flash-grandcanyon.signed
override args.rom_size: 0x15000 => 0x40000
override args.uboot_fit_offset: 0x80000 => 0x100000
override args.uboot_fit_size: 0x60000 => 0xa0000

spl md5sum: 51bef6882caf72f192b6c629bdccb6f7: OK
rec-u-boot md5sum: bf667e4f9773d5262d85458f67122972: OK

Lockbit: not set
SPL FIT offset: 99252
  SPL KEK: key-kek (sha256,rsa4096) (signs image)

Subordinate date: Sat Jul 16 06:01:25 UTC 2022 (0x62d25435)
Subordinate timestamp: 1657951285
Subordinate keys signing key: kek
Subordinate keys hash (sha256): f4745b0706d1ca917df46a5514b47e4261ff3d001384188bd269c924b097ad08: OK
  Subordinate: key-subordinate (sha256,rsa4096) (signs conf)

U-Boot date: Sat Jul 16 06:13:45 UTC 2022 (0x62d25719)
U-Boot timestamp: 1657952025
U-Boot configuration signing key: subordinate
U-Boot hash (sha256): b8c93294983f0b90bf163338e0e12fbe1683220584d011d5c69d774cf541c545: OK

Kernel/ramdisk date: Sat Jul 16 06:13:53 UTC 2022 (0x62d25721)
Kernel/ramdisk timestamp: 1657952033
Kernel/ramdisk configuration signing key: subordinate
Kernel hash (sha256): 1198d31de3bffbd46c7d2432f793bfae78f93b8460a30b7a49935857b238cc06: OK
Ramdisk hash (sha256): cc1ac8a4cd451a9ccdae702e699559c49b3302d1acb2910acdf8885ada5cc059: OK
```
