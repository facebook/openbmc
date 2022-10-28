project('wdtcli', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
)

deps = [
    dependency('libwatchdog'),
]

wdtcli_exe = executable('wdtcli',
    'wdtcli.c',
    dependencies: deps,
    install: true,
)
