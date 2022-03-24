project('libobmc-pldm', 'c',
    version: '0.1',
    license: 'GPL2',
    default_options: ['werror=true'],
    meson_version: '>=0.40')

install_headers(
    'pldm.h',
    'pldm_base.h',
    'pldm_fw_update.h',
    'pldm_pmc.h',
    subdir: 'openbmc')

cc = meson.get_compiler('c')
libs = [
  cc.find_library('ncsi'),
]

srcs = files(
  'pldm.c',
  'pldm_pmc.c',
)

# PLDM library.
pldm_lib = shared_library('obmc-pldm', srcs,
    dependencies: libs,
    version: meson.project_version(),
    install: true)

# pkgconfig for PLDM library.
pkg = import('pkgconfig')
pkg.generate(libraries: [pldm_lib],
    name: meson.project_name(),
    version: meson.project_version(),
    description: 'library to provide PLDM functionality')
