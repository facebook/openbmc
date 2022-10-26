project('ipmid', 'c',
    version: '0.1',
    license: 'GPL2',
    meson_version: '>=0.40',
)

cc = meson.get_compiler('c')

deps = [
    dependency('libcjson'),
    dependency('libkv'),
    cc.find_library('pthread'),
]


srcs = files(
    'ipmid.c',
    'timestamp.c',
    'sel.c',
    'lan.c',
    'cfg-parse.c',
    'common-handles.c',
)

ipmi_exe = executable('ipmid', srcs,
    dependencies: deps,
    install: true
)

