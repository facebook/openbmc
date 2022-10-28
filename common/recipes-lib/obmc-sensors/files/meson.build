project('libobmc-sensors',
    'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40'
)

cc = meson.get_compiler('cpp')

headers = [
    'obmc-sensors.h',
]

sources = [
    'obmc-sensors.cpp',
    'sensor.cpp',
    'sensorchip.cpp',
    'sensorlist.cpp',
]

deps = [
    cc.find_library('sensors'),
    dependency('threads'),
]

install_headers(
    headers,
    subdir: 'openbmc',
)

sensors_lib = shared_library('obmc-sensors',
    sources,
    dependencies: deps,
    version: meson.project_version(),
    install: true,
)

pkg = import('pkgconfig')
pkg.generate(
    libraries: [ sensors_lib ],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Helper Abstraction to the LM Sensors library'
)
