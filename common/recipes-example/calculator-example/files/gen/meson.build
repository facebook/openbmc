# Generated file; do not modify.
sdbuspp_gen_meson_ver = run_command(
    sdbuspp_gen_meson_prog,
    '--version',
).stdout().strip().split('\n')[0]

if sdbuspp_gen_meson_ver != 'sdbus++-gen-meson version 3'
    warning('Generated meson files from wrong version of sdbus++-gen-meson.')
    warning(
        'Expected "sdbus++-gen-meson version 3", got:',
        sdbuspp_gen_meson_ver
    )
endif

