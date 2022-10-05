branches=(
    rocko
    lf-master
    lf-dunfell
    lf-kirkstone
)

rocko_repos=(
    meta-openembedded:eae996301
    meta-security:74860b2
    poky:5f660914cd # bitbake: bitbake-user-manual: Fixed section head typo
)
rocko_poky_patch="0001-rocko-backport-support-for-override.patch 0002-Remove-checks-on-python.patch"
lf_master_repos=(
    lf-openbmc:1361b328c
)
lf_dunfell_repos=(
    lf-openbmc:cddccf4ad
)
lf_kirkstone_repos=(
    lf-openbmc:53fdac2b0
)
