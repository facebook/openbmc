## Open BMC Verified Boot Tests

This is a set of integration tests for Open BMC's verified-boot built using U-Boot's verified-boot features.

### Signing example

The scripts in `tools/signing` can be used to sign the firmware.

Requirements:
- Ubuntu: `sudo apt install python-pip && sudo pip install -U pip && sudo pip install jinja2 pycrypto`
- CentOS: `sudo yum install python-jinja2 python-crypto`

We assume you have built an Open BMC firmware, for example `fbtp`.
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

From the `tools/signing` directory:
```sh
cd tools/signing
```

Create the ROM's key-enrollment-key (KEK) compiled DTB:
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

Sign the subordinate-key compiled DTB using the KEK:
```sh
./fit-signsub \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --keydir /tmp/kek \
  /tmp/subordinate/subordinate.dtb /tmp/subordinate/subordinate.dtb.signed
```

**This last step should be run for each new flash image**

Sign the firmware using the subordinate key, add the KEK store, and signed subordinate store:
```sh
./fit-sign \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --sign-os \
  --kek /tmp/kek/kek.dtb \
  --signed-subordinate /tmp/subordinate/subordinate.dtb.signed \
  --keydir /tmp/subordinate \
  $POKY_BUILD/tmp/deploy/images/fbtp/flash-fbtp $POKY_BUILD/tmp/deploy/images/fbtp/flash-fbtp.signed
```

### Hardware tokens

It is possible to sign with a hardware token. We use the Nitrokey Pro as an example PKCS11 device and include steps to provision a 4096-bit RSA keypair and use the device to sign an Open BMC image.

Several hardware tokens are used in production. One is used to sign a set of subordinate ODM public keys. The private key for those subordinates are also held on hardware tokens. If a subordinate key is lost it can be removed by re-signing the set of subordinate keys and not including the lost key.

**Provision 4096-bit keys on Nitrokey Pro with gnupg2**

Use the following UDEV rules as `40-nitrokey.rules`:
```
# Nitrokey U2F
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0664", GROUP="plugdev", ATTRS{idVendor}=="2581", ATTRS{idProduct}=="f1d0"
# Nitrokey FIDO U2F
KERNEL=="hidraw*", SUBSYSTEM=="hidraw", MODE="0664", GROUP="plugdev", ATTRS{idVendor}=="20a0", ATTRS{idProduct}=="4287"

SUBSYSTEM!="usb", GOTO="gnupg_rules_end"
ACTION!="add", GOTO="gnupg_rules_end"

# USB SmartCard Readers
## Crypto Stick 1.2
ATTR{idVendor}=="20a0", ATTR{idProduct}=="4107", ENV{ID_SMARTCARD_READER}="1", ENV{ID_SMARTCARD_READER_DRIVER}="gnupg", GROUP+="plugdev", TAG+="uaccess"
## Nitrokey Pro
ATTR{idVendor}=="20a0", ATTR{idProduct}=="4108", ENV{ID_SMARTCARD_READER}="1", ENV{ID_SMARTCARD_READER_DRIVER}="gnupg", GROUP+="plugdev", TAG+="uaccess"
## Nitrokey Storage
ATTR{idVendor}=="20a0", ATTR{idProduct}=="4109", ENV{ID_SMARTCARD_READER}="1", ENV{ID_SMARTCARD_READER_DRIVER}="gnupg", GROUP+="plugdev", TAG+="uaccess"
## Nitrokey Start
ATTR{idVendor}=="20a0", ATTR{idProduct}=="4211", ENV{ID_SMARTCARD_READER}="1", ENV{ID_SMARTCARD_READER_DRIVER}="gnupg", GROUP+="plugdev", TAG+="uaccess"
## Nitrokey HSM
ATTR{idVendor}=="20a0", ATTR{idProduct}=="4230", ENV{ID_SMARTCARD_READER}="1", ENV{ID_SMARTCARD_READER_DRIVER}="gnupg", GROUP+="plugdev", TAG+="uaccess"

LABEL="gnupg_rules_end"
```

Then copy it to UDEV's rule set:
```
sudo cp 40-nitrokey.rules /etc/udev/rules.d/
```

Install the UDEV rules and `gpg2` on Ubuntu 16.04:
```sh
sudo service udev restart
sudo apt install pcscd gnupg2 scdaemon
```

Install the UDEV rules and `pcsc-lite-libs` on CentOS7:
```
sudo udevadm control --reload-rules && sudo udevadm trigger
sudo yum install pcsc-lite-libs pcsc-lite-ccid
```

On CentOS7 apply the patch to `/lib64/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist`:
```
$ cat Info.plist.patch
--- /lib64/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist 2016-11-05 21:55:16.000000000 +0000
+++ ./Info.plist  2017-06-11 05:08:59.699000000 +0000
@@ -364,6 +364,10 @@
 		<string>0x08C3</string>
 		<string>0x08C3</string>
 		<string>0x15E1</string>
+		<string>0x20A0</string>
+		<string>0x20A0</string>
+		<string>0x20A0</string>
+		<string>0x20A0</string>
 	</array>
 
 	<key>ifdProductID</key>
@@ -631,6 +635,10 @@
 		<string>0x0401</string>
 		<string>0x0402</string>
 		<string>0x2007</string>
+		<string>0x4108</string>
+		<string>0x4109</string>
+		<string>0x4211</string>
+		<string>0x4230</string>
 	</array>
 
 	<key>ifdFriendlyName</key>
@@ -898,6 +906,10 @@
 		<string>Precise Biometrics Precise 250 MC</string>
 		<string>Precise Biometrics Precise 200 MC</string>
 		<string>RSA RSA SecurID (R) Authenticator</string>
+		<string>Nitrokey Pro</string>
+		<string>Nitrokey Storage</string>
+		<string>Nitrokey Start</string>
+		<string>Nitrokey HSM</string>
 	</array>
 
 	<key>Copyright</key>
$ sudo patch -p1 /lib64/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist < ./Info.plist.patch
```

Verify the Nitrokey Pro is attached and detected:
```sh
$ gpg2 --card-status
Reader ...........: Nitrokey Nitrokey Pro (XXXXXX) 00 00
Application ID ...: XXXXXX
Version ..........: 2.1
Manufacturer .....: ZeitControl
[...]
```

For a new Nitrokey Pro, change the Admin (PW3) and User (PW1) pin.
The default Admin/PW3 pin is: **12345678**, the default User/PW1 pin is **123456**.
```sh
gpg2 --change-pin
```

You will need the Admin pin to create the key, or change it.
```
$ gpg2 --card-edit
gpg/card> admin
Admin commands are allowed

gpg/card> generate
Make off-card backup of encryption key? (Y/n) n

gpg: Note: keys are already stored on the card!

Replace existing keys? (y/N) y
What keysize do you want for the Signature key? (2048) 4096
The card will now be re-configured to generate a key of 4096 bits
What keysize do you want for the Encryption key? (2048) 4096
The card will now be re-configured to generate a key of 4096 bits
What keysize do you want for the Authentication key? (2048) 4096
The card will now be re-configured to generate a key of 4096 bits
Please specify how long the key should be valid.
         0 = key does not expire
[...]
Key is valid for? (0) 
Key does not expire at all
Is this correct? (y/N) y

GnuPG needs to construct a user ID to identify your key.

Real name: OBMC-KEK1
Email address: 
Comment: 
You selected this USER-ID:
    "OBMC-KEK1"

Change (N)ame, (C)omment, (E)mail or (O)kay/(Q)uit? o
```

We will not be using the `gpg-agent` so feel free to:
```
killall gpg-agent
```

**Use the Nitrokey Pro to sign manually**

Install required packages for Ubuntu:
```
sudo apt install gnutls-bin libengine-pkcs11-openssl opensc opensc-pkcs11
export OPENSSL_ENGINES=/usr/lib/ssl/engines
export PKCS11_MODULE=/usr/lib/x86_64-linux-gnu/opensc-pkcs11.so
```

Install required packages for CentOS7:
```
sudo yum install gnutls-utils engine_pkcs11 opensc
export OPENSSL_ENGINES=/usr/lib64/openssl/engines/
export PKCS11_MODULE=/usr/lib64/opensc-pkcs11.so
```

If the Nitrokey Pro is attached (see above) the following `p11tool` can verify tokens:
```
$ p11tool --provider $PKCS11_MODULE --list-tokens
Token 0:
	URL: pkcs11:model=PKCS%2315%20emulated;manufacturer=ZeitControl;serial=XXXXXXXXXXXX;token=OpenPGP%20card%20%28User%20PIN%20%28sig%29%29
	Label: OpenPGP card (User PIN (sig))
	Type: Hardware token
	Manufacturer: ZeitControl
	Model: PKCS#15 emulated
	Serial: 000500004fed


Token 1:
	URL: pkcs11:model=PKCS%2315%20emulated;manufacturer=ZeitControl;serial=XXXXXXXXXXXX;token=OpenPGP%20card%20%28User%20PIN%29
	Label: OpenPGP card (User PIN)
	Type: Hardware token
	Manufacturer: ZeitControl
	Model: PKCS#15 emulated
	Serial: 000500004fed

```

The `mkimage` in the Open BMC U-Boot knows to use a hardware token when an environment variable is set. Use the signing private key URL as the key option:
```
./fit-signsub \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --token "XXXXXXXXXXXX" --token-hint 'kek' \
  /tmp/subordinate/subordinate.dtb /tmp/subordinate/subordinate.dtb.signed
```

**Signing public keys from subordinate Nitrokey Pros**

A hardware token should be used to hold the KEK private key. This means it should be used to sign subordinate key stores. Normally the public keys in the subordinate store will have their private keys on other hardware tokens. This means the public keys from each non-KEK hardware token must be collected and stored for signing.

To show the public key for a hardware token:
```
p11tool --provider $PKCS11_MODULE --export-pubkey "pkcs11:model=PKCS%2315%20emulated;manufacturer=ZeitControl;serial=XXXXXXXXXXXX;token=OpenPGP%20card%20%28User%20PIN%20%28sig%29%29;id=%01;object=Signature%20key" --login
```

To use this output as the KEK:
```
mkdir -p /tmp/kek
p11tool [...] --outfile /tmp/kek/kek.pub
```

Create the certificate-store the same was as above, with the non-hardware token KEK. Do the same to create a subordinate key and key store. Then use the `fit-signsub` example above.

As a simulation, we'll have a non-hardware token KEK, and hardware token subordinate alongside a non-hardware token subordinate. Create the KEK and subordinate directories as described above. Then use the `p11tool` to write the public key for a hardware token into the subordinate directory.
```
p11tool [...] --outfile /tmp/subordinate/hw.pub
```

When `fit-cs --subordinate [...]` is run, it will output '2 keys' written. Sign this with the KEK using `fit-signsub`. Then use `fit-sign` and the hardware-token to sign U-Boot:
```
./fit-sign \
  --mkimage $POKY_BUILD/tmp/sysroots/x86_64-linux/usr/bin/mkimage \
  --kek /tmp/kek/kek.dtb \
  --signed-subordinate /tmp/subordinate/subordinate.dtb.signed \
  --token "XXXXXXXXXXXX" --token-hint 'odm0' \
  INPUT_FLASH OUTPUT_FLASH.signed
```


### TODO

- Write the R/W flash environment to enforce verified-boot in software-mode.
- Include another EDK model to enforce verified-boot in hardware-mode.
- Allow the model to reboot and inspect SRAM for retries.
