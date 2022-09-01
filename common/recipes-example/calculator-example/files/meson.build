project('calculator-example', 'cpp')

sdbusplus_dep = dependency('sdbusplus')
sdbusplusplus_prog = find_program('sdbus++', native: true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)

generated_sources = []
generated_others = []
subdir('gen')
subdir('gen/net')

executable(
    'calculator-example',
    'main.cpp',
    generated_sources,
    dependencies: sdbusplus_dep,
    include_directories: include_directories('gen'),
    install: true
)
