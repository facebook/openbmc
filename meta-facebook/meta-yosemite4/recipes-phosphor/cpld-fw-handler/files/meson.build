project('cpld-fw-handler', 'cpp',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++20',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>= 0.57.0',
    license: 'Apache-2.0',
    version: '1.0')

executable(
    'cpld-fw-handler',
    'cpld-fw-main.cpp',
    'cpld-fw-handler.cpp',
    'cpld-fw-handler.hpp',
    'cpld-lattice.cpp',
    'cpld-lattice.hpp',
    install: true
)
