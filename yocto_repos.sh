branches=(
    rocko
    zeus
    lf-master
    lf-dunfell
    lf-honister
)

rocko_repos=(
    meta-openembedded:eae996301
    meta-security:74860b2
    poky:5f660914cd # bitbake: bitbake-user-manual: Fixed section head typo
)
rocko_poky_patch="0001-rocko-backport-support-for-override.patch"
zeus_repos=(
    meta-openembedded:2b5dd1eb8
    meta-security:52e83e6
    poky:d88d62c20d # selftest/signing: Ensure build path relocation is safe
)
zeus_poky_patch="0001-zeus-backport-support-for-override.patch"
lf_master_repos=(
    lf-openbmc:0ee50bdd6
)
lf_dunfell_repos=(
    lf-openbmc:fff6b3483
)
lf_honister_repos=(
    lf-openbmc:89e2f5ce9
)
