project('vboot-utils', 'c',
    version: '3.3',
    license: 'BSD',
    default_options: [
        'warning_level=3',
        'werror=true',
    ],
)

# The libfdt from Rocko doesn't supply the pkgconfig file, so if we can't
# find it that way use the compiler fallback.
fdt_dep = dependency('libfdt', required: false)
if not fdt_dep.found()
  fdt_dep = meson.get_compiler('c').find_library('fdt')
endif

deps = [
    fdt_dep,
    dependency('libkv'),
    dependency('libvbs'),
]

vboot_util_exe = executable('vboot-util',
    'vboot-util.c',
    dependencies: deps,
    install: true,
)
