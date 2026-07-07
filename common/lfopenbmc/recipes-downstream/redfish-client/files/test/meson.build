gtest_dep = dependency('gtest', main: true, disabler: true, required: false)
gmock_dep = dependency('gmock', disabler: true, required: false)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ],
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').enabled(),
            'Googletest is required if tests are enabled',
        )
    endif
endif

tests = {
    'async_http_client_tests': ['helper.cpp', 'async_http_client_tests.cpp'],
    'dbus_server_tests': ['dbus_server_tests.cpp', 'helper.cpp'],
    'eventlog_parser_tests': ['eventlog_parser_tests.cpp'],
    'log_service_handler_tests': ['log_service_handler_tests.cpp'],
    'persist_map_tests': ['persist_map_tests.cpp'],
    'redfish_binding_common_tests': ['redfish_binding_common_tests.cpp'],
    'redfish_binding_tests': ['redfish_binding_tests.cpp'],
    'sensor_tests': ['sensor_tests.cpp'],
    'config_tests': ['config_tests.cpp'],
}

foreach name, sources : tests
    test(
        name,
        executable(
            'test-' + name,
            sources,
            dependencies: [gtest_dep, gmock_dep, base_deps],
            cpp_args: cpp_args,
            link_args: link_args,
            link_with: redfish_client_library,
            include_directories: ['../', '../include/'],
        ),
    )
endforeach
