project('prot-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++17',
        'werror=true'
    ]
)

srcs = files(
    'prot-util.cpp',
)

deps = [
  dependency('obmc-pal')
]

prot_util_exe = executable(
    'prot-util',
    srcs,
    dependencies: deps,
    install: true,
)
