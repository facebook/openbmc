# OpenBMC tests2 platform tests

The Python (unittest / CIT) tests under this directory run **only for
non-LF platforms** (legacy Facebook OpenBMC). LF (lf-openbmc) platforms
are no longer tested here.

## LF (lf-openbmc) platforms: use the Robot suite

LF platforms are tested by the upstream openbmc-test-automation Robot
suite, executed in CI via Skycastle (the `cit_openbmc_lf` and
`cit_openbmc_lf_qemu` nodes), not by these tests2 CIT tests. The
per-platform tests2 directories for LF platforms have been removed.

To add or change coverage for an LF platform, add Robot tests under:

    fbsource/third-party/openbmc-test-automation/master/src/facebook/

and enable or disable them per platform (and per QEMU vs hardware) in the
configerator config:

    configerator/source/openbmc/tests/lf_robot_tests/config.cconf

The lists are exclude-based: a discovered test runs everywhere unless a
platform `--exclude`s its tag. See
`fbcode/openbmc/openbmc-test-automation/CLAUDE.md` for how to run the
Robot suite against a BMC booted in QEMU.

LF platforms (tested via Robot, not here): anacapa, bletchley,
bletchley15, catalina, clemente, harma, minerva, santabarbara, ventura,
yosemite4, yosemite5.
