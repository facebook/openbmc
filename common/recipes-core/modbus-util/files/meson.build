#Copyright(c) Meta Platforms, Inc.and affiliates.

project(
    'modbus-util',
    'cpp',
    version : '0.2',
    license : 'GPL2',
    default_options : [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version : '>=0.40',
)

cc = meson.get_compiler('cpp')

modbus_util = executable(
    'modbus-util',
    'modbus-util.cpp',
    dependencies : [
        dependency('libmisc-utils'),
    ],
    install : true,
)
