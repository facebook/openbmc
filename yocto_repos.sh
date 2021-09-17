branches=(
    rocko
    zeus
    lf-master
    lf-dunfell
    lf-gatesgarth
    lf-hardknott
)

rocko_repos=(
    meta-openembedded:eae996301
    meta-security:74860b2
    poky:5f660914cd # bitbake: bitbake-user-manual: Fixed section head typo
)
zeus_repos=(
    meta-openembedded:2b5dd1eb8
    meta-security:52e83e6
    poky:d88d62c20d # selftest/signing: Ensure build path relocation is safe
)
lf_master_repos=(
    lf-openbmc:e6866cac1
)
lf_dunfell_repos=(
    lf-openbmc:fff6b3483
)
lf_gatesgarth_repos=(
    lf-openbmc:56721c97f
)
lf_hardknott_repos=(
    lf-openbmc:d767d3fb1
)
