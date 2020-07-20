branches=(
    rocko
    warrior
    zeus
)

rocko_repos=(
    meta-openembedded:eae996301
    meta-security:74860b2
    poky:5f660914cd # bitbake: bitbake-user-manual: Fixed section head typo
)
warrior_repos=(
    meta-openembedded:a24acf94d
    meta-security:4f7be0d
    poky:d865ce7154 # python3: Upgrade 3.7.4 -> 3.7.5
)
zeus_repos=(
    meta-openembedded:9e60d3066
    meta-security:ecd8c30
    poky:ca9dd4b8ea # mesa: fix meson configure fix when 'dri' is excluded from PACKAGECONFIG
)
