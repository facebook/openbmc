	project('expander-util',
	    'c',
	    version: '0.1',
	    license: 'GPL2',
	    default_options: [
	        'warning_level=3',
	        'werror=true',
	    ],
	    meson_version: '>=0.40',
	)

	sources = [
	    'expander-util.c',
	]

	add_project_arguments('-D_XOPEN_SOURCE', language: 'c')

	deps = [
	    dependency('libexp'),
	    dependency('obmc-pal'),
	]

	exe = executable('expander-util',
	    sources,
	    dependencies: deps,
	    install: true,
	)
