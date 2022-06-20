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
)

install_headers(
    'instance_id.hpp',
)

deps = [
    dependency('libpldm'),
    dependency('libpldm-oem'),
]

# pldmd executable.
pldmd_exe = executable(
    'pldmd',
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin'
)
