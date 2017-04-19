## Open BMC Verified Boot Tests

This is a set of integration tests for Open BMC's verified-boot built using U-Boot's verified-boot features.

The included `Dockerfile` builds:
- U-Boot from Open BMC
- QEMU with a basic model for the ASPEED AST2500 EDK
- Several forms of Open BMC flash content

The forms built are designed as input to test cases.
The final execution in the `Dockerfile` runs the `tests.py` test harness.

### How to test

Install docker on Ubuntu (or your favorite OS):
- `apt-get install docker.io`

Run the build:
- `docker build .; echo $?`

A return code == 0 is success, otherwise check stdout/stderr for the inconsistent expectation.

### Signing example

The scripts in `./signing` can be used to correctly-sign the firmware.

Requirements:
- Ubuntu: `pip install -U pip && pip install jinja2 pycrypto`
- CentOS: `yum install python-jinja2 python-crypto`

We also assume you have built an OpenBMC firmware, for example `fbtp`.
Set an environment variable for your poky build directory:
```sh
export POKY_BUILD=~/poky/build
```

**The following steps are 1-time setup commands**

First create two pairs of public/private keys:
```sh
mkdir -p /tmp/kek
openssl genrsa -F4 -out /tmp/kek/kek.key 4096
openssl rsa -in /tmp/kek/kek.key -pubout > /tmp/kek/kek.pub

mkdir -p /tmp/subordinate
openssl genrsa -F4 -out /tmp/subordinate/subordinate.key 4096
openssl rsa -in /tmp/subordinate/subordinate.key -pubout > /tmp/subordinate/subordinate.pub
```

From the `signing` directory:
```sh
cd ./signing
```

Create the ROM's key-enrollment-key compiled DTB:
```sh
./fit-cs --template ./store.dts.in \
  /tmp/kek /tmp/kek/kek.dtb
```

Create the rarely-signed subordinate-key compiled DTB:
```sh
./fit-cs --template ./store.dts.in \
  --subordinate --subtemplate ./sub.dts.in \
  /tmp/subordinate /tmp/subordinate/subordinate.dtb
```

Sign the subordinate-key compiled DTB:
```sh
./fit-signsub \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --keydir /tmp/kek \
  /tmp/subordinate/subordinate.dtb /tmp/subordinate/subordinate.dtb.signed
```

**This last step should be run for each new flash image**

Sign the firmware and add the KEK store, and signed subordinate store:
```sh
./fit-sign \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --kek /tmp/kek/kek.dtb \
  --signed-subordinate /tmp/subordinate/subordinate.dtb.signed \
  --keydir /tmp/subordinate \
  $POKY_BUILD/tmp/deploy/images/fbtp/flash-fbtp $POKY_BUILD/tmp/deploy/images/fbtp/flash-fbtp.signed
```

### TODO

- Write the R/W flash environment to enforce verified-boot in software-mode.
- Include another EDK model to enforce verified-boot in hardware-mode.
- Allow the model to reboot and inspect SRAM for retries.
- Include the TPM emulator and patches to QEMU to use userland TPM emulator sockets.
