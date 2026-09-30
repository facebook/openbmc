project('drive-fault-monitor', 'cpp',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23',
  ])

sdbusplus_dep = dependency('sdbusplus')
phosphorlog_dep = dependency('phosphor-logging')

executable(
  'drive-fault-monitor',
  'drive-fault-monitor.cpp',
  dependencies: [sdbusplus_dep, phosphorlog_dep],
  install: true,
  install_dir: get_option('bindir'),
)

systemd_dep = dependency('systemd')
systemd_system_unit_dir = systemd_dep.get_variable(
  pkgconfig: 'systemdsystemunitdir')

install_data(
  'drive-fault-monitor.service',
  install_dir: systemd_system_unit_dir,
)