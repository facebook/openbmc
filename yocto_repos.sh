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
rocko_poky_patch="0001-rocko-backport-support-for-override.patch"
lf_master_repos=(
    lf-openbmc:595d5424d
)
lf_dunfell_repos=(
    lf-openbmc:ab475af38
)
lf_kirkstone_repos=(
    lf-openbmc:cb2a94c39
)
