project('libocpdbg-lcd', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'ocp-dbg-lcd.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  dependency('obmc-i2c'),
  dependency('libipmb'),
  dependency('libipmi'),
]

srcs = files(
  'ocp-dbg-lcd.c',
)

# ocpdbg-lcd library.
ocpdbglcd_lib = shared_library('ocpdbg-lcd', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for CPLD library.
pkg = import('pkgconfig')
pkg.generate(libraries: [ocpdbglcd_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Library providing common features for the LCD USB debug card')
