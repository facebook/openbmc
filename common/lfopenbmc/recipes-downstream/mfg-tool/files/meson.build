project(
  'mfg-tool',
  'cpp',
  version: '1.0',
  default_options: [
    'buildtype=debugoptimized',
    'cpp_std=c++23',
    'warning_level=3',
    'werror=true',
  ],
  meson_version: '>=1.1.1',
)

CLI11_dep = dependency('CLI11', include_type: 'system')
nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')
sdbusplus_dep = dependency('sdbusplus')

registry_dir = phosphor_dbus_interfaces_dep.get_variable('registry_dir')
config_data = configuration_data()
config_data.set('REGISTRY_DIR', registry_dir)

configure_file(
  input: 'config.meson.hpp',
  output: 'config.hpp',
  configuration: config_data,
)

executable(
  'mfg-tool',
  'mfg-tool.cpp',
  'cmd/bmc-arch.cpp',
  'cmd/bmc-state.cpp',
  'cmd/leakdetector-display.cpp',
  'cmd/fan-leader.cpp',
  'cmd/fan-mode.cpp',
  'cmd/fan-speed.cpp',
  'cmd/hello.cpp',
  'cmd/inventory.cpp',
  'cmd/log-clear.cpp',
  'cmd/log-display.cpp',
  'cmd/log-registry.cpp',
  'cmd/log-resolve.cpp',
  'cmd/postcode-display.cpp',
  'cmd/power-control.cpp',
  'cmd/power-state.cpp',
  'cmd/sensor-display.cpp',
  'cmd/valve-control.cpp',
  'cmd/version-display.cpp',
  'utils/json.cpp',
  'utils/mapper.cpp',
  'utils/redfish-registry.cpp',
  'utils/register.cpp',
  dependencies: [
    CLI11_dep,
    nlohmann_json_dep,
    phosphor_dbus_interfaces_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
  ],
  install: true
)
