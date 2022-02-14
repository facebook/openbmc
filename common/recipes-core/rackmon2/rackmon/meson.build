# Copyright 2021-present Facebook. All Rights Reserved.
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
    'Device.cpp',
    'ModbusCmds.cpp',
    'Modbus.cpp',
    'Msg.cpp',
    'UARTDevice.cpp',
    'ModbusDevice.cpp',
    'Register.cpp',
    'Rackmon.cpp',
    'RackmonSock.cpp',
)
srcs = common + files(
    'RackmonSvcUnix.cpp',
)
test_srcs = common + files(
    'tests/MsgTest.cpp',
    'tests/DeviceTest.cpp',
    'tests/ModbusCmdsTest.cpp',
    'tests/ModbusTest.cpp',
    'tests/RegisterDescriptorTest.cpp',
    'tests/RegisterValueTest.cpp',
    'tests/RegisterTest.cpp',
    'tests/RegisterMapTest.cpp',
    'tests/ModbusDeviceTest.cpp',
    'tests/PollThreadTest.cpp',
    'tests/RackmonTest.cpp',
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
  files('RackmonCliUnix.cpp', 'RackmonSock.cpp'),
  dependencies: deps,
  install: true,
  install_dir: 'local/bin'
)

rackmond_test = executable('test-rackmond', test_srcs,
  dependencies: test_deps,
  install_dir: 'lib/rackmon/ptest',
  cpp_args: ['-I.', '-D__TEST__'],
)
test('rackmond-tests', rackmond_test)
