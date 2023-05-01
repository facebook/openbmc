project('pldmd', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

srcs = files(
    'pldmd.cpp',
    'instance_id.cpp',
    'fd_handler.cpp',
    'instance_id.cpp',
    'pldm_fd_handler.cpp',

)

cc = meson.get_compiler('cpp')
deps = [
    dependency('libpldm'),
    dependency('libpldm-oem'),
    cc.find_library('glog'),
]

# pldmd executable.
pldmd_exe = executable(
    'pldmd',
    srcs,
    dependencies: deps,
    cpp_args: ['-DNDEBUG'],
    install: true,
    install_dir : 'local/bin'
)

util_srcs = files(
    'pldmd-util.cpp'
)

# pldmd executable.
pldm_util = executable(
    'pldmd-util',
    util_srcs,
    dependencies: deps,
    install: true,
    install_dir : 'bin'
)
