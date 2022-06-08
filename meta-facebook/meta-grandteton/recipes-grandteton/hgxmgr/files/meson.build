project('hgxmgr', 'c', 'cpp',
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
    'main.cpp',
)

cc = meson.get_compiler('c')
deps = [
  dependency('threads'),
  cc.find_library('restclient-cpp'),
]

hgxmgr_exe = executable(
    'hgxmgr', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin',
)
