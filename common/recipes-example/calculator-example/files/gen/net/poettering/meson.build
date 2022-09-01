# Generated file; do not modify.
subdir('Calculator')
generated_others += custom_target(
    'net/poettering/Calculator__markdown'.underscorify(),
    input: [ '../../../yaml/net/poettering/Calculator.interface.yaml',  ],
    output: [ 'Calculator.md' ],
    command: [
        sdbuspp_gen_meson_prog, '--command', 'markdown',
        '--output', meson.current_build_dir(),
        '--tool', sdbusplusplus_prog,
        '--directory', meson.current_source_dir() / '../../../yaml',
        'net/poettering/Calculator',
    ],
)

