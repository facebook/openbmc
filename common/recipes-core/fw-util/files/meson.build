project('fw-util',
    'cpp',
    version: '0.2',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

cc = meson.get_compiler('cpp')

# The libfdt from Rocko doesn't supply the pkgconfig file, so if we can't
# find it that way use the compiler fallback.
fdt_dep = dependency('libfdt', required: false)
if not fdt_dep.found()
  fdt_dep = cc.find_library('fdt')
endif

srcs = [
    'image.cpp',
    'bmc.cpp',
    'check_image.cpp',
    'extlib.cpp',
    'nic.cpp',
    'pfr_bmc.cpp',
    'scheduler.cpp',
    'server.cpp',
    'spiflash.cpp',
    'system.cpp',
    'system_mock.cpp',
    'tpm.cpp',
    'tpm2.cpp',
    'vr.cpp',
]

if get_option('fscd')
    srcs += [
        'fscd.cpp',
    ]
endif

if get_option('bic')
    srcs += [
        'bic_bios.cpp',
        'bic_cpld.cpp',
        'bic_fw.cpp',
        'bic_me.cpp',
    ]
endif

deps = [
    cc.find_library('dl', required: false),
    dependency('libcrypto'),
    dependency('libgpio-ctrl'),
    dependency('libkv'),
    dependency('libmisc-utils'),
    dependency('libobmc-i2c'),
    dependency('libvbs'),
    dependency('obmc-pal'),
    dependency('threads'),
    dependency('zlib'),
    fdt_dep,
]

subdir('plat')

exe = executable(
    'fw-util',
    'fw-util.cpp',
    srcs,
    dependencies: deps,
    install: true,
)

test_srcs = [
    'tests/bmc-test.cpp',
    'tests/fw-util-test.cpp',
    'tests/nic-test.cpp',
]

test_deps = [
    cc.find_library('gmock'),
    cc.find_library('gtest'),
    cc.find_library('gtest_main'),
]

test_exe = executable(
    'test-fw-util',
    'fw-util.cpp',
    srcs,
    test_srcs,
    cpp_args: '-D__TEST__',
    dependencies: [deps, test_deps],
)
test('fw-util-tests', test_exe)
