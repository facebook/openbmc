project('mterm',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'tty_helper.c',
    'mTerm_helper.c',
]

deps = [
]

internal_lib = static_library('mterm',
    sources,
    dependencies: deps,
    install: false,
)

server = executable('mTerm_server',
    'mTerm_server.c',
    link_with: internal_lib,
    dependencies: deps,
    install: true,
)

client = executable('mTerm_client',
    'mTerm_client.c',
    link_with: internal_lib,
    dependencies: deps,
    install: true,
)
