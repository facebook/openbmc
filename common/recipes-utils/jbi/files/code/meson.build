project('jbi',
    'c',
    version: '2.2',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

sources = [
    'jbicomp.c',
    'jbijtag.c',
    'jbimain.c',
    'jbistub.c',
]

deps = [
    dependency('liblog'),
    dependency('libgpio-ctrl'),
]

exe = executable('jbi',
    sources,
    dependencies: deps,
    install: true,
)
