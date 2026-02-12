project(
  'bios-fw-update',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  license: 'Apache-2.0',
  default_options: [
    'werror=true',
    'warning_level=3',
    'cpp_std=c++20',
  ])

conf_data = configuration_data()

conf_data.set_quoted('VENDOR_NAME', get_option('vendor-name'))
conf_data.set_quoted('PLATFORM_NAME', get_option('platform-name'))
conf_data.set_quoted('BOARD_NAME', get_option('board-name'))
conf_data.set('IANA_NUMBER', get_option('iana-number'))

configure_file(output: 'config.hpp', configuration: conf_data)

libpldm_dep = dependency(
    'libpldm',
    fallback: ['libpldm', 'libpldm_dep'],
    include_type: 'system',
)

phosphor_logging_dep = dependency('phosphor-logging')

libpldmutils_headers = ['pldmutils']
libpldmutils = library(
    'pldm-utils',
    'pldmutils/utils.cpp',
    'pldmutils/package_parser.cpp',
    version: meson.project_version(),
    dependencies: [
        libpldm_dep,
        phosphor_logging_dep,
    ],
    install: true,
    include_directories: include_directories(libpldmutils_headers),
)

libpldmutils = declare_dependency(
    include_directories: include_directories(libpldmutils_headers),
    link_with: libpldmutils,
)

cc = meson.get_compiler('cpp')

dep_libs = [
  dependency('boost'),
  dependency('sdbusplus'),
  dependency('libusb-1.0'),
  dependency('phosphor-logging'),
  dependency('phosphor-dbus-interfaces'),

  libpldmutils,
]

src = [
  'bios-update.cpp',
  'bios-usb-update.cpp',
]

executable(
  'bios-fw-update',
  src,
  dependencies: dep_libs,
  install: true,
)
