# Copyright 2023-present Meta Platform. All Rights Reserved.
project('libwedge-eeprom', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

install_headers(
     'WeutilInterface.h',
    subdir: 'facebook')

deps = [
  dependency('liblog'),
  dependency('libobmc-i2c'),
  ]

# source files for libwedge-eeprom.
libsrcs = files(
    'WeLibApi.cpp',
    'CrcLib.h',
    'Eeprom.h',
    'Eeprom.cpp',
    'Cfg.h',
    'Cfg.cpp',
    'WeutilInterface.h',
    'FbossEeprom.h',
    'FbossEeprom.cpp',
    'FbossEepromParser.h',
    'FbossEepromParser.cpp',
    )

# weutil library
weutil_lib = shared_library('wedge-eeprom', libsrcs,
    dependencies: deps,
    version: meson.project_version(),
    install: true)

# pkgconfig for weutil library.
pkg = import('pkgconfig')
pkg.generate(libraries: [weutil_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'wedge eeprom utility library')

