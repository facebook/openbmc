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

add_project_arguments('-Wno-psabi', language: 'cpp')

common = files(
    'Device.cpp',
    'ModbusCmds.cpp',
    'Modbus.cpp',
    'Msg.cpp',
    'UARTDevice.cpp',
    'ModbusDevice.cpp',
    'Register.cpp',
    'Rackmon.cpp',
    'UnixSock.cpp',
)
srcs = common + files(
    'RackmonSvcUnix.cpp',
)
test_srcs = common + files(
    'tests/Main.cpp',
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
    'tests/UnixSockTest.cpp',
)

cc = meson.get_compiler('cpp')
deps = [
  dependency('threads'),
  cc.find_library('glog'),
]

if get_option('syslog') == true
    add_global_arguments('-DRACKMON_SYSLOG=1', language : 'cpp')
endif

if get_option('profiling') == true
    add_global_arguments('-DPROFILING=1', language : 'cpp')
    deps += [ dependency('libmisc-utils') ]
    install_data('scripts/rackmon-stress.py', install_dir: '/usr/local/bin', install_mode: 'rwxrwxrwx')
endif

if cc.get_id() == 'gcc' and cc.version().version_compare('<9')
    deps += [ cc.find_library('stdc++fs') ]
endif
test_deps = deps + [
  cc.find_library('gtest'),
  cc.find_library('gmock'),
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
  files('RackmonCliUnix.cpp', 'UnixSock.cpp'),
  dependencies: deps,
  install: true,
  install_dir: 'local/bin'
)


interface_configs = [
  'aspeed_uart.conf',
  'usb_ft232.conf',
  'usb_ft4232.conf',
]

register_maps = [
  'orv2_psu.json',
]
if get_option('orv3') == true
  register_maps += [
    'orv3_psu.json',
    'orv3_bbu.json',
  ]
endif

filepaths_interface = []
filepaths_regmap = []
foreach c : interface_configs
  file = join_paths('configs', 'interface', c)
  install_data(file, install_dir: '/etc')
  filepaths_interface += [file]
endforeach
foreach c : register_maps
  file = join_paths('configs', 'register_map', c)
  install_data(file, install_dir: '/etc/rackmon.d')
  filepaths_regmap += [file]
endforeach

config_validator = files('scripts/schema_validator.py')
confchecker_iface = custom_target(
  'check_syntax_iface',
  command: [
    'python3',
    config_validator,
    files('schemas/InterfaceConfigSchema.json'),
    '@INPUT@',
  ],
  input: files(filepaths_interface),
  depend_files: files(filepaths_interface),
  build_by_default: true,
  capture: true,
  output: 'validate_configs_inteface.log',
)
confchecker_regmap = custom_target(
  'check_syntax_regmap',
  command: [
    'python3',
    config_validator,
    files('schemas/RegisterMapConfigSchema.json'),
    '@INPUT@',
  ],
  input: files(filepaths_regmap),
  depend_files: files(filepaths_regmap),
  build_by_default: true,
  capture: true,
  output: 'validate_configs_regmap.log',
)

pyrmd_tests = join_paths(meson.source_root(), meson.current_source_dir(), 'tests', 'test_pyrmd.py')
pyrmd_tester = custom_target(
  'check_pyrmd',
  command: [
    'python3',
    '@INPUT@',
  ],
  input: files(pyrmd_tests),
  depend_files: files(pyrmd_tests),
  build_by_default: true,
  capture: true,
  output: 'validate_pyrmd.log',
)

rackmond_test = executable('test-rackmond', test_srcs,
  dependencies: test_deps,
  install_dir: 'lib/rackmon/ptest',
  cpp_args: ['-I.', '-D__TEST__'],
)
test('rackmond-tests', rackmond_test)
