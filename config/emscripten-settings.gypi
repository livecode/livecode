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

		'c++_std': '<!(echo ${CXX_STD:-c++11})',
	},

	'defines':
	[
		'__GNU__',
	],

	'cflags':
	[
		'-s ALLOW_MEMORY_GROWTH=1',
		'-s ASSERTIONS=1',
		'-s DEMANGLE_SUPPORT=1',
		'-s EMTERPRETIFY=1',
		'-s EMTERPRETIFY_ASYNC=1',
		'-s LINKABLE=1',
		'-s RESERVED_FUNCTION_POINTERS=1024',
		'-s TOTAL_MEMORY=67108864',
		'-s WARN_ON_UNDEFINED_SYMBOLS=1',
	],

	'cflags_c':
	[
		'-std=gnu99',
		'-Wstrict-prototypes',
		'-Werror=declaration-after-statement',	# Ensure C89 compliance
	],

	'cflags_cc':
	[
		'-fno-exceptions',
		'-fno-rtti',
		'-std=<(c++_std)',
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

					'-Werror=conversion-null',
					'-Werror=logical-not-parentheses',
					'-Werror=return-type',
					'-Werror=tautological-compare',
					'-Werror=uninitialized',
				],
				'cflags_cc':
				[
					'-Werror=mismatched-tags',
					'-Werror=overloaded-virtual',
					'-Werror=delete-non-virtual-dtor',
				],
			},
			{
				'cflags':
				[
					'-w',
					'-fpermissive',
					'-Wno-return-type',
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
				'-O2',
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
				'-Os',
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
