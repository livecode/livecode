{
	'variables':
	{
		'mobile': 1,
		'output_dir': '../emscripten-js-bin',
	},

	'target_defaults':
	{
		'variables':
		{
			'app_bundle_suffix': '',
			'ext_bundle_suffix': '.so',
			'lib_suffix': '.so',
			'ext_suffix': '.so',
			'exe_suffix': '.js',
			'debug_info_suffix': '.dbg',

			'c++_std': 'gnu++03',
		},

		'defines':
		[
			'__GNU__',
		],

		'cflags_cc':
		[
			'-fno-exceptions',
			'-fno-rtti',
		],

		'cflags_c':
		[
			'-std=gnu99',
			'-Wstrict-prototypes',
		],

		'cflags':
		[
			'-Wall',
			'-Wextra',
		],

		'ar_flags':
		[
			# llvm-ar doesn't understand "T" (i.e. "thin archive")
			'crs',
		],

		'target_conditions':
		[
			[
				'_toolset == "target"',
				{
					'cflags':
					[
						'-O2',
						'-s ASSERTIONS=1',
						'-s DEMANGLE_SUPPORT=1',
						'-s EMTERPRETIFY=1',
						'-s EMTERPRETIFY_ASYNC=1',
						'-s WARN_ON_UNDEFINED_SYMBOLS=1',
						'-s ALLOW_MEMORY_GROWTH=1',
					],
				},
			],
		],

		'configurations':
		{
			'Debug':
			{
				'cflags':
				[
					'-g3',
				],

				'defines':
				[
					'_DEBUG',
				],
			},

			'Release':
			{
				'cflags':
				[
					'-O3',
					'-g3',
				],

				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},

			'Fast':
			{
				'cflags':
				[
					'-O0',
					'-g0',
				],

				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},
		},
	},
}