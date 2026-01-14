project('ftdi-mdio', 'c',
    version: '0.1',
    default_options: ['c_std=gnu99', 'werror=true'])

libusb  = dependency('libusb-1.0', required: true)
libftdi = dependency('libftdi1',   required: true) 

executable('ftdi-mdio',
  ['ftdi_mdio.c'],
  dependencies: [libusb, libftdi],
  install: true,
  install_dir: get_option('bindir')
)
