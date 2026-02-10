project('mdio-us-mac', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
)

wdtcli_exe = executable('mdio-us-mac',
    'mdio_us_mac.c',
    link_args: ['-static'],
    install: true,
)
