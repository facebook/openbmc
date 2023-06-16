project('retimer-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++20',
        'werror=true',
    ],
    meson_version: '>=0.40',
)
srcs = files(
    'retimer-util.cpp',
)

cc = meson.get_compiler('cpp')
deps = [
  dependency('libretimer'),
  dependency('obmc-pal'),
]

hgxmgr_exe = executable(
    'retimer-util', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin',
)
