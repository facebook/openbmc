project('libbic', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40')


# OpenSSL3, which is present in Kirkstone and above, deprecates some
# SHA hash functions we are using.  Turn off the warning until we can
# port to OpenSSL3.
add_project_arguments('-Wno-error=deprecated-declarations', language: 'c')

add_project_arguments('-D ENABLE_INJECTION', language: 'c')

cc = meson.get_compiler('c')

headers = [
    'bic.h',
    'bic_xfer.h',
    'bic_power.h',
    'bic_ipmi.h',
    'bic_fwupdate.h',
    'bic_cpld_altera_fwupdate.h',
    'bic_cpld_lattice_fwupdate.h',
    'bic_vr_fwupdate.h',
    'bic_bios_fwupdate.h',
    'bic_mchp_pciesw_fwupdate.h',
    'bic_brcm_pciesw_fwupdate.h',
    'bic_m2_fwupdate.h',
]

sources = [
    'bic_xfer.c',
    'bic_power.c',
    'bic_ipmi.c',
    'bic_fwupdate.c',
    'bic_cpld_altera_fwupdate.c',
    'bic_cpld_lattice_fwupdate.c',
    'bic_vr_fwupdate.c',
    'bic_bios_fwupdate.c',
    'bic_bios_usb_fwupdate.c',
    'bic_mchp_pciesw_fwupdate.c',
    'bic_brcm_pciesw_fwupdate.c',
    'bic_m2_fwupdate.c',
]

deps = [
    cc.find_library('fby3_common'),
    dependency('libkv'),
    dependency('libusb-1.0'),
    dependency('libipmb'),
    dependency('openssl'),
    dependency('libobmc-i2c'),
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
