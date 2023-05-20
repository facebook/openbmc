The system should be using the Yocto included in [Linux Foundation
OpenBMC][lf-openbmc] and based on the `master` branch. All changes necessary for
the system should be sent upstream into the appropriate meta-layer or code
repository as it is developed with specific exceptions:

- Commits which have been submitted upstream but are not yet merged may be
  carried as a patch in the [Meta Fork][fb-openbmc], if the functionality is
  critical for a development milestone, until the patch is merged upstream.
  Patches must first be submitted upstream and the carried patch file must
  contain a Gerrit `Change-Id` or Lore `LORE LINK` identifier pointing to the
  upstream submission.

- Code which must be kept private due to NDAs may be placed into the Meta fork's
  ODM repository under the proprietary tree.

All exceptions need to be approved by the Meta BMC system owner and
non-proprietary patches should be upstreamed prior to Pilot phase.
