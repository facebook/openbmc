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

if get_option('update-ebr-init').enabled()
    add_project_arguments('-DENABLE_UPDATE_EBR_INIT', language: 'cpp')
    summary('update-ebr-init', '-DENABLE_UPDATE_EBR_INIT', section: 'Enabled Features')
endif

executable(
    'cpld-fw-handler',
    'cpld-fw-main.cpp',
    'cpld-fw-handler.cpp',
    'cpld-fw-handler.hpp',
    'cpld-lattice.cpp',
    'cpld-lattice.hpp',
    install: true
)
