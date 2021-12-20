project('rackmon', 'cpp',
    version: '0.2',
    license: 'GPL2',
    # Meson 0.40 only supports c++1z as an alias for c++17.
    default_options: [
        'cpp_std=c++1z',
        'warning_level=3',
        'werror=true',
    ],
    meson_version: '>=0.40',
)

common = files(
    'dev.cpp',
    'modbus_cmds.cpp',
    'modbus.cpp',
    'msg.cpp',
    'uart.cpp',
    'modbus_device.cpp',
    'regmap.cpp',
    'rackmon.cpp',
    'rackmon_sock.cpp',
)
srcs = common + files(
    'rackmon_svc_unix.cpp',
)
test_srcs = common + files(
    'tests/msg_test.cpp',
    'tests/dev_test.cpp',
    'tests/modbus_cmds_test.cpp',
    'tests/modbus_test.cpp',
)

cc = meson.get_compiler('cpp')
deps = [
  dependency('threads'),
]

if get_option('syslog') == true
    add_global_arguments('-DRACKMON_SYSLOG=1', language : 'cpp')
    deps += [ cc.find_library('log') ]
endif

if get_option('profiling') == true
    add_global_arguments('-DPROFILING=1', language : 'cpp')
    deps += [ cc.find_library('misc-utils') ]
endif

if cc.get_id() == 'gcc' and cc.version().version_compare('<9')
    deps += [ cc.find_library('stdc++fs') ]
endif
test_deps = deps + [
  cc.find_library('gtest'),
  cc.find_library('gmock'),
  cc.find_library('gtest_main'),
]

rackmond_exe = executable(
    'rackmond', 
    srcs,
    dependencies: deps,
    install: true,
    install_dir : 'local/bin'
)
rackmon_cli_exe = executable(
  'rackmoncli',
  files('rackmon_cli_unix.cpp', 'rackmon_sock.cpp'),
  dependencies: deps,
  install: true,
  install_dir: 'local/bin'
)

rackmond_test = executable('test-rackmond', test_srcs,
  dependencies: test_deps,
  install_dir: 'lib/rackmon/ptest',
  cpp_args: ['-I.'],
)
test('rackmond-tests', rackmond_test)
