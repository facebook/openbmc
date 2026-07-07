project('ftdi-mdio', 'c',
    version: '0.1',
    default_options: ['c_std=gnu99', 'werror=true'])

libusb  = dependency('libusb-1.0', required: true)
libftdi = dependency('libftdi1', required: true) 
libpthread = dependency('threads', required: true)

core_sources = ['ftdi_mdio_core.c']
executable('ftdi-mdio',
    sources: ['ftdi_mdio_cli.c'] + core_sources,
    dependencies: [libusb, libftdi],
    install: true,
    install_dir: get_option('bindir')
)
executable('ftdi-mdio-daemon',
    sources: ['ftdi_mdio_daemon.c'] + core_sources,
    dependencies: [libusb, libftdi, libpthread],
    install: true,
    install_dir: get_option('bindir')
)