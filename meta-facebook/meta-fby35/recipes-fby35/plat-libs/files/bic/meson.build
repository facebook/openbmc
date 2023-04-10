project('libbic',
    'c', 'cpp',
    version: '0.1',
    license: 'LGPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40')

# OpenSSL3, which is present in Kirkstone and above, deprecates some
# SHA hash functions we are using.  Turn off the warning until we can
# port to OpenSSL3.
add_global_arguments('-Wno-error=deprecated-declarations', language: 'c')

headers = [
    'bic.h',
    'bic_bios_fwupdate.h',
    'bic_cpld_altera_fwupdate.h',
    'bic_fwupdate.h',
    'bic_ipmi.h',
    'bic_power.h',
    'bic_xfer.h',
]

sources = [
    'bic_bios_fwupdate.c',
    'bic_bios_usb_fwupdate.c',
    'bic_cpld_altera_fwupdate.c',
    'bic_fwupdate.c',
    'bic_ipmi.c',
    'bic_power.c',
    'bic_xfer.c',
]

deps = [
    dependency('libcrypto'),
    dependency('libfby35-common'),
    dependency('libgpio-ctrl'),
    dependency('libipmb'),
    dependency('libkv'),
    dependency('libobmc-i2c'),
    dependency('libusb-1.0'),
    dependency('libpldm'),
    dependency('libpldm-oem')
]

install_headers(
    headers,
    subdir: 'facebook',
)

bic_lib = shared_library('bic',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ bic_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Bridge IC library'
)
