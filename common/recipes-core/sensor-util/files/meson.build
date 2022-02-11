project('sensor-util', 'c', 'cpp',
    version: '0.1',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

srcs = files(
    'sensor-util.cpp',
)

cc = meson.get_compiler('c')
deps = [
  dependency('threads'),
  dependency('libaggregate-sensor'),
  cc.find_library('pal'),
  cc.find_library('sdr'),
  cc.find_library('jansson'),
]

sensor_util_exe = executable(
    'sensor-util', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin'
)
