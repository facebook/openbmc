# Rackmon / Modbus Device Firmware Upgrade — What Changed

This document describes the updates to the rack component firmware upgrade flow
relative to the process documented in
[OpenBMC/Runbook/Rackmon/Rackmon: Rack Component Firmware Upgrade](https://www.internalfb.com/wiki/OpenBMC/Runbook/Rackmon/RackmonFirmwareUpgrade/).

The short version: there is now **one** upgrade command,
`/usr/local/bin/modbus-update.py`. Point it at a device — by the same `--addr`
as before, or by name — and it works out what the device is, which vendor built
it, and which updater to run. The old per-vendor `*-update-*.py` scripts still
exist and still take `--addr`, but they are deprecated wrappers.

---

## The bus lock

Every firmware upgrade must run under the modbus bus lock, which guarantees
exclusive access to the bus so no other tool performs read/write operations on
the modbus device mid-upgrade.

**`modbus-update.py` takes the lock itself.** It holds `/run/lock/modbus.lock`
from before it resolves the device until it exits, so run it bare. If another
tool holds the lock, it prints `Waiting for /run/lock/modbus.lock` and blocks
until the lock is released, the same as `flock` does.

rackmond creates `/run/lock/modbus.lock` at startup and makes the legacy
`/tmp/modbus_dynamo_solitonbeam.lock` a symlink to it, so both paths are the
same lock.

Still wrapping it in `flock /tmp/modbus_dynamo_solitonbeam.lock` (or
`flock /run/lock/modbus.lock`) is harmless. `modbus-update.py` sees that it
already inherited the lock from `flock` and uses that lock instead of waiting
for it. The exception is `flock -o`, which closes its descriptor before running
the command: `modbus-update.py` would then wait on its own parent forever.
Don't combine the two.

The old per-vendor scripts do **not** take the lock. They still have to be run as

```
flock /tmp/modbus_dynamo_solitonbeam.lock <old upgrade command>
```

## The part that did not change

Nothing validates that you are pointing a firmware image at a device that should
receive it. The new flow removes the "wrong script for the vendor" class of
mistake, but pointing it at the wrong *image* can still brick a device.

---

## New flow: `modbus-update.py`

```
/usr/local/bin/modbus-update.py (-n <NAME> | -a <UDA>) [-c <COMPONENT>] \
    [--dry-run] [--force-direct] <path>
```

| Argument | Meaning |
|---|---|
| `<path>` | Firmware file (positional, required) |
| `-n`, `--name` | Device name, `<TYPE>_<position>[_<device number>]` |
| `-a`, `--address` / `--addr` | Rackmon unique device address. Forces the rackmon backend |
| `-c`, `--component` | Sub-component, for devices that have them (RPU, RPU2) |
| `--dry-run` | Resolve the device, detect the vendor, pick the updater, then stop before writing anything |
| `--force-direct` | Use the direct (minimalmodbus) backend even for a rackmon-managed device |

Exactly one of `-n` and `-a` is required — they are two ways of naming the
same device, so giving neither and giving both are both usage errors.

* **`-a` (the old-school way).** Rackmon already scanned the bus and
  matched the device against a register map, so it is asked what the device is:
  `ORV3_HPR_PMM_BBU` at that address means a BBU PMM, and the Panasonic-or-Delta
  question is then settled by reading the part. This is the drop-in replacement
  for the old `--addr` commands, and `--addr` abbreviates `--address`, so the
  flag is spelled the same as before. It is also the only way to reach a device
  the naming scheme does not place on the bus.
* **`-n`.** For phosphor-modbus devices, whose names entity-manager exports and
  whose address is not yours to know, and for legacy ORv3 parts whose address
  the name derives.

### Device names

`decode_name()` parses `<TYPE>_<position>[_<device number>]`, upper-casing the
type. Supported types: `PSU`, `BBU`, `CBU`, `RPU`, `PSU_PMM`, `BBU_PMM`,
`CBU_PMM`.

Position encodes which stack the device lives on:

* **position >= 100 → legacy ORv3 / rackmon.** Positions 100-102 are racks 1-3.
  `RPU_100`-`RPU_102` are RPUs of either generation — an AALCv1 PLC/HEX RPU
  and an AALCv2 one (RPU2) share those shelves, so which is which is settled
  at runtime, see below. Only `PSU`, `BBU` and `RPU` can be placed on the bus
  by name, so a bare `CBU_100_1` or `BBU_PMM_100` is rejected — reach those
  with `-a`.
* **position < 100 → phosphor-modbus.** The name is looked up directly against
  the device configs entity-manager exports, and the direct minimalmodbus
  backend is used.

Passing `-a/--address` forces the rackmon path. On that path, and on the
rackmon path a position >= 100 takes, the device type is always rackmon's own
rather than the one the name says:

| Rackmon register map | Device type |
|---|---|
| `ORV3_PSU` | `ORV3_PSU` |
| `ORV3_BBU` | `ORV3_BBU` |
| `ORV3_RPU` | `RPU` |
| `ORV3_RPU2` | `RPU2` |
| `ORV3_HPR_PSU` | `PSU` |
| `ORV3_HPR_BBU` | `BBU` |
| `ORV3_HPR_CBU` | `CBU` |
| `ORV3_HPR_PMM_PSU` | `PSU_PMM` |
| `ORV3_HPR_PMM_BBU` | `BBU_PMM` |
| `ORV3_HPR_PMM_CBU` | `CBU_PMM` |

`ORV2_PSU` and `MINIUPS` have no updater here and are rejected.

For a PSU or a BBU addressed by name, the unique device address is derived from
that name:

| Type | Unique device address |
|---|---|
| `PSU_<pos>_<n>` | `((rack+1) << 8) \| (3 << 6) \| (1 << 5) \| (rack << 3) \| (n-1)`, rack = pos - 100 |
| `BBU_<pos>_<n>` | `((rack+1) << 8) \| (1 << 6) \| (rack << 3) \| (n-1)`, rack = pos - 100 |

The derived address is then looked up in `rackmond`'s device list; an address
rackmon does not know about is an error.

An RPU's address is not derived — `RPU_<pos>` alone says nothing about which
generation is installed, and the two are not laid out the same way on the bus.
`rpu_rackmon_uaddr()` reads `rackmond`'s device list and takes the address from
whichever RPU it matched there:

* **AALCv1** (`ORV3_RPU`) puts one RPU on each rack. A unique device address is
  `port << 8 | address` and the port is the rack, so the shelf picks between
  them: `RPU_100`, `RPU_101` and `RPU_102` are three different RPUs.
* **AALCv2** (`ORV3_RPU2`) is a cooling pod: one modbus device — the master
  controller — fronts every rack assembly in the pod. oobit reports the pod's
  components across shelves 100-102 so they do not collide with each other, but
  they are all that one device, so `RPU_100`, `RPU_101` and `RPU_102` all reach
  it. Which assembly gets written is `-c`'s job, not the shelf's.

No RPU for that shelf is an error.

### Vendor detection

The vendor is no longer something you assert on the command line — it is read
off the device. `manufacturers.get_manufacturer()` reads the discriminator
register described in the modbus-device-util config and matches it against the
per-vendor `registerValues` / `registerRegex` for that device type:

| Config key | Device type | Vendors |
|---|---|---|
| `psu` | `ORV3_PSU` | artesyn, delta |
| `bbu` | `ORV3_BBU` | delta, panasonic |
| `rpu` | `RPU` | delta, quanta |
| `rpu2` | `RPU2` | coolermaster |
| `hpr_psu` | `PSU` | artesyn, delta |
| `hpr_bbu` | `BBU` | delta, panasonic |
| `hpr_cbu` | `CBU` | delta |
| `hpr_pmm_psu` | `PSU_PMM` | delta, artesyn |
| `hpr_pmm_bbu` | `BBU_PMM` | delta, panasonic |
| `hpr_pmm_cbu` | `CBU_PMM` | delta |

Config resolution order (shared with the modbus-device-util shell tooling):

1. `$MODBUS_DEVICE_UTIL_CONFIG`, if set
2. `/run/mnt-persist/var-data/lib/modbus-device-util/override-config.json`, if present
3. `/var/lib/modbus-device-util/default-config.json`

An override on the persistent partition outlives the image it was dropped on, so
if the override does not know the device type being upgraded, the shipped
default config is used instead (with a printed note).

### Examples

```
# ORv3 PSU, rack 1 slot 3 — vendor (delta or artesyn) auto-detected
/usr/local/bin/modbus-update.py -n PSU_100_3 /tmp/psu.bin

# ORv3 BBU, rack 2 slot 1
/usr/local/bin/modbus-update.py -n BBU_101_1 /tmp/bbu.bin

# AALCv1 RPU, PLC (default component)
/usr/local/bin/modbus-update.py -n RPU_100 /tmp/rpu.bin

# AALCv1 RPU, HEX
/usr/local/bin/modbus-update.py -n RPU_100 -c HEX /tmp/hex.bin

# AALCv2 RPU — same name, rackmon says which generation is on the shelf.
# Component derived from the image filename if -c is omitted. The whole pod is
# one device, so RPU_101 and RPU_102 reach it too
/usr/local/bin/modbus-update.py -n RPU_100 /tmp/MT-R_P.tar.gz
/usr/local/bin/modbus-update.py -n RPU_100 -c FAN_RACK_1_ETH /tmp/MT-E_F1.tar.gz

# HPR PSU / BBU / CBU / PMMs behind phosphor-modbus
/usr/local/bin/modbus-update.py -n PSU_1_1 /tmp/psu.bin
/usr/local/bin/modbus-update.py -n BBU_1_2 /tmp/bbu.bin
/usr/local/bin/modbus-update.py -n CBU_1_1 /tmp/cbu.bin
/usr/local/bin/modbus-update.py -n BBU_PMM_1 /tmp/pmm.bin

# Anything rackmon knows, by address alone — the type comes from rackmon
/usr/local/bin/modbus-update.py -a 0x1e0 /tmp/psu.bin
/usr/local/bin/modbus-update.py --addr 0x1e0 /tmp/psu.bin

# Check what would happen without touching the device
/usr/local/bin/modbus-update.py -n PSU_100_3 --dry-run /tmp/psu.bin
/usr/local/bin/modbus-update.py -a 0x1e0 --dry-run /tmp/psu.bin
```

`--dry-run` prints the resolved device, detected vendor, backend and updater —
use it before any upgrade you are unsure about.

### AALCv2 (RPU2) components

`-c` accepts `PUMP_RACK_{ETH,RPU,UPSCOM,UPSINV,UPSPFC}`,
`FAN_RACK_1_{ETH,RPU,UPSCOM,UPSINV,UPSPFC}` and
`FAN_RACK_2_{ETH,RPU,UPSCOM,UPSINV,UPSPFC}`. As before, if you preserve
Coolermaster's filename (`MT-E_P.tar.gz`, `UPSCOM_F1.tar.gz`, ...) the component
is derived from the basename and `-c` can be omitted.

---

## Old flow (`--addr`) — still works, deprecated

The hyphenated scripts are kept as thin backwards-compatible wrappers. They take
`--addr <unique device address>` and require you to know the vendor and variant.
This is the flow described on the wiki:

### ORv2

| Device | Command |
|---|---|
| Delta (V2/G2) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-delta.py --addr <addr> <path>` |
| Artesyn (V2/G2) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-artesyn.py --addr <addr> <path>` |
| BEL (V2) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-bel.py --addr <addr> <path>` |

### ORv3 / ORv3-HPR

| Device | Command |
|---|---|
| ORv3 PSU AEI | `... /usr/local/bin/psu-update-aei.py --addr <addr> <path>` |
| ORv3 PSU Delta | `... /usr/local/bin/psu-update-delta-orv3.py --addr <addr> <path>` |
| ORv3 BBU Delta | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` |
| ORv3 BBU Panasonic | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor panasonic --addr <addr> <path>` |
| HPR PMM AEI | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_pmm_aei --addr <addr> <path>` |
| HPR PMM Panasonic | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_pmm_panasonic --addr <addr> <path>` |
| HPR PMM Delta | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_pmm_delta --addr <addr> <path>` |
| HPR PSU AEI | `... /usr/local/bin/psu-update-aei.py --addr <addr> --device hpr <path>` |
| HPR PSU Delta | `... /usr/local/bin/psu-update-delta-orv3.py --addr <addr> <path>` |
| HPR BBU Delta | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` |
| HPR BBU Panasonic | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_panasonic --addr <addr> <path>` |
| CBU Delta | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` |
| CBU PMM Delta | `... /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_pmm_delta --addr <addr> <path>` |
| RPU PLC Delta | `... /usr/local/bin/rpu-update-delta-plc.py --addr <addr> --oem-block <path>` |
| RPU PLC Quanta | `... /usr/local/bin/rpu-update-delta-plc.py --addr <addr> <path>` |
| RPU HEX Delta | `... /usr/local/bin/rpu-update-delta-hex.py --addr <addr> <path>` |
| RPU AALCv2 Coolermaster | `... /usr/local/bin/rpu-update-coolermaster.py --addr <addr> <path>` |

(RPU address is usually `0xc`, AALCv2 `0xd`; RMC uses the unique device address.)

Minimum firmware versions from the wiki still apply: HPR PMM AEI `0001G`, HPR PMM
Panasonic `00.01.16`, HPR PSU AEI `004` (with PMM >= `0001G`), HPR PSU Delta
`12003310` / `01_02_00_06`, HPR BBU Delta `S1.03B02`, HPR BBU Panasonic
`02.19.15`, CBU Delta `v20912091`, CBU PMM Delta `v2030`.

---

## Old → new command

One row per vendor/platform/component combination on the wiki, in the same
order. `<addr>` is the rackmon unique device address, `<path>` the firmware file.
The new commands drop the `flock` wrapper, since `modbus-update.py` takes the
lock itself.

| Component | Old command | New command |
|---|---|---|
| Delta ORv2 PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-delta.py --addr <addr> <path>` | *no equivalent — keep the old command* |
| Artesyn ORv2 PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-artesyn.py --addr <addr> <path>` | *no equivalent — keep the old command* |
| BEL ORv2 PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-bel.py --addr <addr> <path>` | *no equivalent — keep the old command* |
| Artesyn (AEI) ORv3 PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-aei.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta ORv3 PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-delta-orv3.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta ORv3 BBU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Panasonic ORv3 BBU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor panasonic --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Panasonic ORv3 BBU, very old (64-byte blocks) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor panasonic --block-size 64 --addr <addr> <path>` | *no equivalent — keep the old command* |
| Artesyn (AEI) HPR PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-aei.py --addr <addr> --device hpr <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR PSU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/psu-update-delta-orv3.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR BBU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Panasonic HPR BBU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor hpr_panasonic --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR CBU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --vendor delta --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Artesyn (AEI) HPR PSU PMM | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_aei <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR PSU PMM | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_delta <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Panasonic HPR BBU PMM | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_panasonic <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR BBU PMM | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_delta <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR CBU PMM | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_delta <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta HPR PMM, fw < `1000` (64-byte blocks) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/orv3-device-update-mailbox.py --addr <addr> --vendor hpr_pmm_delta --block-size 64 <path>` | *no equivalent — keep the old command* |
| Delta ORv3 RPU (PLC) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/rpu-update-delta-plc.py --addr <addr> --oem-block <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Quanta ORv3 RPU (PLC) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/rpu-update-delta-plc.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Delta ORv3 RPU (HEX) | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/rpu-update-delta-hex.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> --component HEX <path>` |
| Coolermaster AALCv2 RPU | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/rpu-update-coolermaster.py --addr <addr> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> <path>` |
| Coolermaster AALCv2 RPU, renamed image | `flock /tmp/modbus_dynamo_solitonbeam.lock /usr/local/bin/rpu-update-coolermaster.py --addr <addr> --component <C> <path>` | `/usr/local/bin/modbus-update.py --addr <addr> --component <C> <path>` |

Every row keeps the `<addr>` the old command took, since `--name` and `--addr`
are mutually exclusive and the address is the one already in the runbook. Where
a name does exist it would do just as well: it only says where to look, and what
the device turns out to be is rackmon's answer either way.

The wiki spells some of these `-vendor` with a single dash. That does not parse
— argparse reads `-vendor` as the short option `-v`, which no updater defines,
and exits with "unrecognized arguments". Use `--vendor`.

`<addr>` is usually `0xc` for an AALCv1 RPU and `0xd` for an AALCv2 one; RMC
uses the unique device address. Named instead, `RPU_100` is rack 1 for AALCv1,
whose racks 2 and 3 are `RPU_101` and `RPU_102`; an AALCv2 pod is one device
and answers to all three names.

Minimum versions from the wiki still apply: HPR PSU PMM AEI `0001G`, HPR PMM
Panasonic `00.01.16`, HPR PSU AEI `004` (PMM must be >= `0001G`), HPR PSU Delta
`12003310` (`01_02_00_06`), HPR BBU Delta `S1.03B02`, HPR BBU Panasonic
`02.19.15`, HPR CBU Delta `v20912091`, HPR CBU PMM Delta `v2030`.

As before, the AALCv2 image must keep the filename Coolermaster shipped
(`MT-E_P.tar.gz`, `MT-R_F1.tar.gz`, `UPSCOM_F2.tar.gz`, ...) unless `--component`
is given: the basename is what tells the updater which of the six components is
being written.

### On RMCv2 / phosphor-modbus

Where there is no rackmon address to give, the same devices are addressed by the
name entity-manager exports and `--addr` is dropped:

| Component | New command |
|---|---|
| HPR PSU (shelves 1-4, slots 1-6) | `/usr/local/bin/modbus-update.py --name PSU_1_1 <path>` |
| HPR BBU (shelves 1-4, slots 1-6) | `/usr/local/bin/modbus-update.py --name BBU_1_1 <path>` |
| HPR CBU (shelves 1-10, slots 1-3) | `/usr/local/bin/modbus-update.py --name CBU_1_1 <path>` |
| HPR PSU PMM (shelves 1-4) | `/usr/local/bin/modbus-update.py --name PSU_PMM_1 <path>` |
| HPR BBU PMM (shelves 1-4) | `/usr/local/bin/modbus-update.py --name BBU_PMM_1 <path>` |
| HPR CBU PMM (shelves 1-10) | `/usr/local/bin/modbus-update.py --name CBU_PMM_1 <path>` |

The vendor is detected either way, so there is one command per component rather
than one per vendor. Shelf and slot ranges above are ventura2's
`allowed-devices.json`.


### Behaviour differences worth knowing

* **There is no key to pass.** Both flows fall back to the key built into
  `delta_key.py`; `psu-update-delta-orv3.py --key` was always optional and
  `modbus-update.py` does not expose it at all. On an image where that key is
  not populated the update aborts with "PSU Update Key is needed to upgrade
  this device".
* **CBU now uses the `delta_cbu` mailbox profile**, not `delta`. Same 64-byte
  block size, but `block_wait` is off. The wiki's `--vendor delta` for CBU was
  the ORv3 BBU profile being reused.
* **RPU PLC OEM-block is derived from the vendor.** Delta gets `--oem-block`
  behaviour, Quanta does not — you no longer pass the flag.
* **There is no block size override.** Every device still in the field takes
  its vendor's default — ORv3 Panasonic BBU 96, ORv3 Delta BBU 64, HPR Panasonic
  BBU 96, all HPR PMMs 68 — so the new flow always uses it. The wiki's two
  legacy escape hatches (very old Panasonic BBUs, and Delta PMMs on firmware
  below `1000`) stay on `orv3-device-update-mailbox.py --block-size 64`.
* **The bus lock is automatic.** `modbus-update.py` takes
  `/run/lock/modbus.lock` itself, dry runs included, since they read the
  vendor off the device. See [The bus lock](#the-bus-lock).
* **Monitoring suppression is automatic.** `modbus-update.py` wraps the whole
  update in `dev.suppress_monitoring()`, which pauses rackmond (or
  phosphor-modbus) polling of the device and, for devices behind a PMM, the
  PMM's own polling — on exit including exit by exception.
* **On the rackmon path, rackmon says what the device is.** A name only ever
  decides *where to look*; the type comes from the register map rackmon matched
  when it probed the device. That is what tells `ORV3_HPR_PSU` from `ORV3_PSU`,
  which no name in the rackmon scheme can express. A device rackmon has not
  placed — `deviceType` of `Unknown`, or an `ORV2_PSU` — is refused rather than
  guessed at.
* **A bare `--addr` is the closest thing to the old commands.** Same flag, same
  value, different script. Prefer it on RMCv1, and keep `--name` for
  phosphor-modbus, where there is no address to give.

---

## `firmware-upgrade` (oobit path)

`common/recipes-core/modbus-device-util/files/firmware-upgrade` is the entry
point oobit drives, and it changed shape too:

* It now takes **one** device at a time instead of a list of addresses.
* The second argument is a location in `<position>_<device number>` format
  rather than a raw unique device address. On RMCv1, `position` *is* the unique
  device address and `device_number` is the literal string `NA`; RMCv2 will use
  shelf (100+) and slot. Anything other than `NA` is currently rejected with
  "This upgrade is not supported yet".
* It still resolves the manufacturer through `get-manufacturer` and still
  invokes the per-vendor scripts under
  `flock /tmp/modbus_dynamo_solitonbeam.lock`.

Heads up: the comment in `firmware-upgrade` says RMCv2 positions start at 100,
while `modbus-update.py` treats position >= 100 as *legacy ORv3 on rackmon* and
positions 1..N as phosphor-modbus (matching ventura2's `allowed-devices.json`,
which numbers PSU/BBU shelves 1-4 and CBU shelves 1-10). The two need to agree
before the non-`NA` branch of `firmware-upgrade` is turned on.

---

## Under the hood

* **Scripts became importable modules.** Each `foo-update-bar.py` is now a thin
  wrapper around a `foo_update_bar.py` module exposing `main(dev, file, ...)`.
  `modbus-update.py` imports those modules directly rather than shelling out.
* **The modbus handle is an object, not an address.** Two backends implement the
  same interface: `modbus_impl_pyrmd.Modbus` (through rackmond) and
  `modbus_impl_minimalmodbus.Modbus` (direct on the serial port, used for
  phosphor-modbus devices and for `--force-direct`). Both raise the same
  exception classes.
* **`phosphor_modbus.py`** enumerates devices from entity-manager over D-Bus and
  can exclude phosphor-modbus from a serial port for the duration of an update.
* **Unit tests** ship with the package as ptest
  (`python3 -m unittest discover` under `/usr/local/fbpackages/psu`).
