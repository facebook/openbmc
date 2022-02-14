project('modprobe',
    version: '0.1',
    license: 'GPL2',
    meson_version: '>=0.40')

sources = [
    'modprobe.conf',
]

install_data(sources, install_dir : '/etc/modprobe.d', install_mode : 'rwxrw-rw-')
