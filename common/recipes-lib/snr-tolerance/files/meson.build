project('libsnr-tolerance', 'cpp',
    version: '0.1',
    license: 'GPL2',
    default_options: ['cpp_std=c++17', 'werror=true'],
    meson_version: '>=0.40')

install_headers(
    'snr-tolerance.h',
    subdir: 'openbmc')

libs = []

srcs = files('snr-tolerance.cpp')

# snr-tolerance library.
snr_tolerance_lib = shared_library('snr-tolerance', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

pkg = import('pkgconfig')
pkg.generate(libraries: [snr_tolerance_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Sensor Reading Tolerance Library')
