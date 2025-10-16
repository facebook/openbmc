project(
    'cxl-mailbox',
    'cpp',
    version: '0.1',
    default_options: [
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true'
    ]
)

uuid_dep = dependency('uuid')

executable(
    'cxl-mailbox',
    sources: [
        'cxl-mailbox.cpp',
        'libcxl.cpp'
    ],
    dependencies: [uuid_dep],
    install: true,
    install_dir: get_option('bindir')
)
