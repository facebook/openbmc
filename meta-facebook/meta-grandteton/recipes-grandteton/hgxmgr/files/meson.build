project('hgxmgr', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++20',
        'werror=true',
    ],
    meson_version: '>=0.40',
)
srcs = files(
    'main.cpp',
)

cc = meson.get_compiler('cpp')
deps = [
  dependency('threads'),
  cc.find_library('restclient-cpp'),
  dependency('libhgx'),
]

hgxmgr_exe = executable(
    'hgxmgr', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin',
)
