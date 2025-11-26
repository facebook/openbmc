project(
  'adhoc-sensor',
  'cpp',
  version: '0.1',
  default_options: [
    'buildtype=debugoptimized',
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>=1.1.1',
)

# Configuration options
conf_data = configuration_data()
conf_data.set_quoted(
  'DEFAULT_CHASSIS',
  get_option('default-chassis'),
  description: 'Default chassis path for sensor associations'
)

configure_file(
  output: 'config.h',
  configuration: conf_data
)

boost_dep = dependency('boost', include_type: 'system')
sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

executable(
  'adhoc-sensor',
  'adhoc-sensor.cpp',
  dependencies: [
    boost_dep,
    sdbusplus_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
  ],
  install: true,
  install_dir: get_option('bindir')
)
