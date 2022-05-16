branches=(
    rocko
    lf-master
    lf-dunfell
    lf-honister
    lf-kirkstone
)

rocko_repos=(
    meta-openembedded:eae996301
    meta-security:74860b2
    poky:5f660914cd # bitbake: bitbake-user-manual: Fixed section head typo
)
rocko_poky_patch="0001-rocko-backport-support-for-override.patch"
lf_master_repos=(
    lf-openbmc:d61350c77
)
lf_dunfell_repos=(
    lf-openbmc:e093d3473
)
lf_honister_repos=(
    lf-openbmc:a515de07d
)
lf_kirkstone_repos=(
    lf-openbmc:322e9fc9c
)
