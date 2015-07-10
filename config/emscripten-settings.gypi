# -*-Javascript-*-
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

	'cflags':
	[
		'-s ASSERTIONS=1',
		'-s DEMANGLE_SUPPORT=1',
		'-s WARN_ON_UNDEFINED_SYMBOLS=1',
		'-s ALLOW_MEMORY_GROWTH=1',

		'-s EMTERPRETIFY=1',
		'-s EMTERPRETIFY_ASYNC=1',
	],

	'cflags_c':
	[
		'-std=gnu99',
		'-Wstrict-prototypes',
	],

	'cflags_cc':
	[
		'-fno-exceptions',
		'-fno-rtti',
	],

	'target_conditions':
	[
		[
			'silence_warnings == 0',
			{
				'cflags':
				[
					'-Wall',
					'-Wextra',
					'-Wno-unused-parameter',
					'-Wno-ignored-attributes',
				],
			},
			{
				'cflags':
				[
					'-w',
					'-fpermissive',
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
}
