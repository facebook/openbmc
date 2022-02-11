install_headers('watchdog.h', subdir: 'openbmc')

# Find pthread library
thread_lib = dependency('threads')

# Build watchdog library.
watchdog_lib = shared_library('watchdog',
    'watchdog.c',
    dependencies: thread_lib,
    version: meson.project_version(),
    install: true)

# Create pkgconfig for watchdog.
pkg = import('pkgconfig')
pkg.generate(libraries: [watchdog_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'Watchdog control library')
