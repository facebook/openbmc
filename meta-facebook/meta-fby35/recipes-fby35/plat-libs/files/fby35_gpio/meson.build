project('libfby35-gpio',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

headers = [
    'fby35_gpio.h',
]

sources = [
    'fby35_gpio.c',
]

deps = [
    dependency('libfby35-common'),
    dependency('libbic'),
]

install_headers(
    headers,
    subdir: 'facebook'
)

gpio_lib = shared_library('fby35-gpio',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ gpio_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'FBY35 gpio Library'
)
