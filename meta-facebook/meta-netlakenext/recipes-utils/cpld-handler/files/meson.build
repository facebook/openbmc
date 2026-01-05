project('cpld-handler', 'cpp',
    default_options: [
        'buildtype=debugoptimized',
        'cpp_std=c++17',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>= 0.53.0',
    license: 'Apache-2.0',
    version: '1.0')

if get_option('update-ebr-init').enabled()
    add_project_arguments('-DENABLE_UPDATE_EBR_INIT', language: 'cpp')
    summary('update-ebr-init', '-DENABLE_UPDATE_EBR_INIT', section: 'Enabled Features')
endif

executable(
    'cpld-handler',
    'cpld-main.cpp',
    'cpld-handler.cpp',
    'cpld-handler.hpp',
    'cpld-lattice.cpp',
    'cpld-lattice.hpp',
    install: true
)
