# QEMU master rebase — working notes

Scratch/state file for the QEMU `QEMU_REF` bump + patch re-port. This becomes the
basis for the final diff summary and can be deleted afterward.

## Goal
Bump `meta-facebook` QEMU from the pinned master commit to the latest upstream
master, re-porting the ~43 downstream patches so they apply and build.

## Version facts
- Old pin: `b7d13c3602176b648cdcf351dc0a1c8d5f5aa8cc` (exactly v11.0.0)
- New master: `a59157f98f0b69b0bbdb26bc15fbc4d6c8060799` (v11.0.0-2604-ga59157f98f)
- Jump: ~2604 commits.
- Rebase branch (local clone): `/tmp/qemu-upgrade` branch `fb-rebase`.

## Subproject SRCREV refresh (from *.wrap at new master)
Only ONE changed:
- `SRCREV_libvfio-user`: `0b28d205572c80b568a1003db2c8f37ca333e4d7` -> `4d9f663450fa80ff375612dbbafe073700e3d3d8`
- keycodemapdb, berkeley-softfloat-3, berkeley-testfloat-3, dtc, slirp, libblkio: unchanged.

## How this package is actually built (NOT a full bitbake build)
From `fbsource/www/flib/intern/sandcastle/commands/SandcastleOpenBMCBuilderCommand.php`
(`publish-qemu-fbpkg` target) -> `facebook/qemu/build.sh`:
1. `bitbake qemu-master-system-native -c patch`  (fetch + apply patches ONLY)
2. `./configure --target-list=aarch64-softmmu ... && ninja -C build`  (direct meson build)
3. meson from a pip venv; deps via dnf (libslirp-devel, libfdt-devel).

## Manual compile + verify performed (for the diff summary)
- meson installed in venv: `fbpython -m venv ~/openbmc_venv && pip install meson` (v1.11.1).
  (Network steps run in a tmux pane under the user identity; the agent identity is
   blocked by the fwdproxy destination filter for github/gitlab.)
- Built directly from `/tmp/qemu-upgrade` (rebased tree), mirroring build.sh flags:
  `./configure --target-list=aarch64-softmmu --without-default-features
   --disable-debug-info --enable-tpm --enable-slirp --enable-fdt=internal --disable-docs`
  then `ninja -C build`. Result: full aarch64 build **green** (NINJA_EXIT=0),
  `qemu-system-aarch64` links.
- Verified machines register: `./build/qemu-system-aarch64 -M help | grep <machine>`.
- Verified instantiation: `qemu-system-aarch64 -M <machine> -S -qmp stdio` handshake OK.

## Patch classification (43 total)

### Applied clean / verified (KEEP, refresh offsets)
- 0500 net/slirp mfr-id/oob-eth-addr — **manual conflict fix** (qapi/net.json: upstream
  retyped hostfwd/guestfwd to NetdevUserHostForward/GuestForward; kept both + FB fields).
- 0501-0508 — apply (0502-0508 via 3-way).
- 0509 fbttn, 0510 bus-ID traces — apply.
- 0511 i2c slave event traces — **manual re-port** (aspeed_i2c.c context drift: re-added
  I2C_EVENT[] table + 2 trace_ calls + LOG_UNIMP default; trace-events additions).
- 0512, 0514, 0515, 0516 — apply.
- 0001-0008 (build/env) — apply.

### Machines re-ported + built + registered (the "re-port candidates") — DONE
- 0528 grandteton, 0529 greatlakes, 0530 sandia, 0534 sanmiguel — clean (ride 0508
  `DEFINE_FB_MACHINES` machinery on upstream's reorganized aspeed_ast2600_evb.c).
- 0531 montblanc, 0532 janga — **+1 line each**: `.interfaces = arm_machine_interfaces`.
  Root cause: upstream (Linaro) now FILTERS machines per target binary via `interfaces`
  (include/hw/arm/machines-qom.h). Without it the machine compiles + registers as a QOM
  type but is invisible/unusable in qemu-system-aarch64. 0508 macro already sets it;
  the explicit janga/montblanc TypeInfo entries did not.
- All 6 verified: register in `-M help` AND instantiate via QMP.
- Refreshed patch files staged in `/tmp/reported/`.

### Structural: upstream reorganized the aspeed machine layer
- Monolithic `hw/arm/aspeed.c` split into per-machine files
  (`aspeed_ast2600_<board>.c`).
- `hw/arm/fby35.c` (the dual-SoC BMC+BIC machine) was REMOVED upstream
  (commit `34f634a207 hw/arm: Remove fby35 machine`).
- Upstream now ships a BMC-only `fby35-bmc` (parents off `ast2600-evb`) with basic
  FRUs + i2c + GPIO in `aspeed_ast2600_fby35.c`.

## OPEN DECISION — the fby35/anacapa deferred cluster (10 patches)
These were skipped during the initial `git am`. Classification:

| patch | subject | files | disposition |
|---|---|---|---|
| 0009 | Add Facebook Anacapa BMC machine | aspeed_ast2600_anacapa.c | **DROP?** upstream has anacapa-bmc |
| 0010 | Add BMC machine to build | hw/arm/meson.build | depends on 0009 / restructure |
| 0517 | Expose i2c buses to machine | aspeed_soc_common.c, **fby35.c**, aspeed_i2c.c, aspeed_soc.h, fby35.h | PORT (partial) |
| 0518 | fby35 add bmc/bb/nic FRUs | aspeed.c, gen_eeprom.py | APPLIED (verify still correct) |
| 0519 | fby35-sb-cpld | hw/misc/fby35_sb_cpld.c (+meson/trace/MAINTAINERS) | PORT (FB-only device) |
| 0520 | intel-me | hw/misc/intel_me.c (+meson/trace/MAINTAINERS) | PORT (FB-only device) |
| 0521 | fby35 server board bridge IC | hw/misc/fby35_sb_bic.c, fby35.h | PORT (FB-only device) |
| 0522 | switch fby35/grandcanyon to n25q00 | aspeed_ast2600_evb.c, aspeed_ast2600_fby35.c | APPLIED |
| 0524 | fby35 setup i2c + GPIO | aspeed_ast2600_fby35.c, **fby35.c**, fby35.h | PORT (hard) |
| 0525 | fby35 mobo fru to BIC | **fby35.c** | PORT |
| 0526 | i2c devices to oby35-cl | aspeed_ast2600_fuji.c(?), **fby35.c**, fby35.h | PORT |
| 0527 | don't init fby35-bmc GPIO | aspeed_ast2600_fby35.c | PORT (small) |

**The decision:** upstream ships a *basic* BMC-only fby35. FB's cluster adds a *rich*
dual-SoC sim: a BIC (oby35-cl/oby35-bb), fby35-sb-cpld, intel-me, full i2c tree, custom
FRUs — all built on the now-removed `hw/arm/fby35.c`. FB device models
(fby35_sb_cpld / intel_me / fby35_sb_bic) are NOT upstream.
- Upstream does NOT define oby35-cl / oby35-bb / Fby35State / TYPE_FBY35 (grep empty).
- => Keeping FB's fby35 means re-creating the dual-SoC machine on top of upstream: real
  engineering, several files. Dropping it means adopting upstream's simpler fby35-bmc
  (greatlakes still works — it parents off upstream fby35-bmc).

DECISION (user): full re-port, keep FB sim. DONE — see results below.

### fby35 cluster re-port RESULTS
- 0009 anacapa: **DROPPED** — upstream ships aspeed_ast2600_anacapa.c; anacapa-bmc
  registers from upstream.
- 0010 add-BMC-to-build: **DROPPED** — only added aspeed_ast2600_anacapa.c to
  meson.build, already present upstream.
- Re-added `hw/arm/fby35.c` (new prep patch) + meson.build entry (upstream removed it).
- 0517/0519/0520/0521/0524/0525/0526/0527: applied. Device-model list-file edits
  (MAINTAINERS/meson.build/trace-events) hand-placed due to context drift.
- **Re-port fix folded into 0517**: 0517 adds object_initialize_child("bmc"/"bic") to
  fby35_init but the restored base still creates them in fby35_bmc_init/fby35_bic_init
  -> duplicate-property crash on new QEMU (fatal now, was tolerated before). Removed the
  duplicate creation from the two sub-inits (0517's design inits both SoCs up front).
- VERIFIED: `fby35` (dual-SoC), `fby35-bmc`, `oby35-cl`, `oby35-bb` all build + boot
  (fby35 prints the upstream "deprecated, use ast2700fc" warning but runs).

## fby35 cluster re-port plan (DECISION: full re-port, keep FB sim)
FB's fby35 is a DUAL-SoC machine (`fby35` = ast2600 BMC + ast1030 BIC) in
`hw/arm/fby35.c`, which existed at the old pin (upstream's 203-line file) and was
REMOVED upstream (kept only BMC-only `fby35-bmc` in aspeed_ast2600_fby35.c).
`include/hw/arm/fby35.h` is FB-introduced (by 0517).

Faithful re-port = **re-add `hw/arm/fby35.c`** (restore the upstream-removed file at
the old-pin blob) + its meson.build entry, then apply the FB wiring patches in order:
  0517 (creates fby35.h, edits fby35.c) -> 0519 -> 0520 -> 0521 (device models,
  fby35.h) -> 0524 -> 0525 -> 0526 -> 0527.
Note: restored fby35.c may need small fixups to compile against 2604-commits-newer
headers (aspeed_soc API drift). Build-test at the end (fby35 + oby35-cl).

## Recipe assembly (DONE)
- QEMU_REF -> a59157f98f..., version comment -> v11.0.0-2604-ga59157f98f.
- SRCREV_libvfio-user -> 4d9f6634...
- 42 patch files refreshed in qemu-git/ (all hunk offsets updated to new master).
- NEW patch 0513-aspeed-Re-add-hw-arm-fby35.c.patch (restores upstream-removed file);
  listed in SRC_URI before 0517.
- DROPPED 0009 (anacapa) + 0010 (add-anacapa-to-build): fully upstreamed.
- SRC_URI cross-checked: 42 file:// lines == 42 files on disk, no orphans/missing.

## Final validation (DONE)
- All 42 patches apply cleanly IN SRC_URI ORDER on a fresh origin/master checkout.
- SRC_URI-order tree is byte-IDENTICAL to the verified fb-rebase tree.
- Verified build (aarch64-softmmu, build.sh flags): CONFIGURE=0, NINJA=0.
- Machines register + instantiate: grandteton, greatlakes, sandia, montblanc, janga,
  sanmiguel, fby35 (dual-SoC), fby35-bmc, oby35-cl, oby35-bb, anacapa (upstream),
  cloudripper, grandcanyon, elbert.

## DONE
- [x] fby35 cluster full re-port.
- [x] Recipe QEMU_REF / version comment / SRCREV_libvfio-user.
- [x] Refreshed patches + new restore patch; SRC_URI reconciled; 0009/0010 dropped.
- [x] SRC_URI-order apply + build compile confirmed.
- [ ] Write diff.
