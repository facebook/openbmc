project('crashdump',
    version: '0.1',
    license: 'GPL2',
    meson_version: '>=0.40')

sources = [
    'autodump.sh',
    'COPYING',
    'crashdump_coreid',
    'crashdump_msr',
    'crashdump_pcie',
    'dump.sh',
]

install_data(sources, install_dir : get_option('bindir') , install_mode : 'rwxr-xr-x')
