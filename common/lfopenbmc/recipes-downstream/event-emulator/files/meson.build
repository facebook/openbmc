project(
    'event-emulator',
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

nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')
sdbusplus_dep = dependency('sdbusplus')
phosphor_dbus_interfaces_dep = dependency('phosphor-dbus-interfaces')
phosphor_logging_dep = dependency('phosphor-logging')

executable(
    'event-emulator',
    'event-emulator.cpp',
    'devices/psu.cpp',
    'devices/bbu.cpp',
    'devices/cbu.cpp',
    'utils/device_events.cpp',
    'utils/device_registry.cpp',
    'utils/event_actions.cpp',
    'utils/event_state.cpp',
    include_directories: ['.'],
    dependencies: [
        nlohmann_json_dep,
        sdbusplus_dep,
        phosphor_dbus_interfaces_dep,
        phosphor_logging_dep,
    ],
    install: true,
)
