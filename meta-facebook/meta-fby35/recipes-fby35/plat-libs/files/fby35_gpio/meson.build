project('libfby35_gpio',
    'c',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

cc = meson.get_compiler('c')

headers = [
    'fby35_gpio.h',
]

sources = [
    'fby35_gpio.c',
]

deps = [
    cc.find_library('fby35_common')
]

install_headers(
    headers,
    subdir: 'facebook'
)

gpio_lib = shared_library('fby35_gpio',
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
