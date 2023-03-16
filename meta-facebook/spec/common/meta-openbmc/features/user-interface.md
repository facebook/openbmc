The preferred user interface for all future systems is Redfish and all Meta DC
tooling is expected to begin leveraging Redfish for all system management.  The
legacy design of `foo-util` which are accessible over SSH is no longer
acceptable.

In some cases it may be useful to have CLI available for debug efforts.  These
utilities should not be Meta-specific (or machine specific), but should instead
be contributed upstream as generally applicable debug tools supported by the
open source community.
