{
	'variables':
	{
		'c++_std': '<!(echo ${CXX_STD:-c++11})',
		'android_lib_path%': '<!(echo ${ANDROID_LIB_PATH:+-L${ANDROID_LIB_PATH}})',
	},

	'cflags':
	[
		'-fstrict-aliasing',
		'-fvisibility=hidden',
	],
	
	'cflags_c':
	[
		'-std=gnu99',
		'-Wstrict-prototypes',
	],
	
	'cflags_cc':
	[
		'-std=<(c++_std)',
		'-fno-exceptions',
		'-fno-rtti',
	],
	
	'ldflags':
	[
		'-fuse-ld=bfd',
	],
	
	'target_conditions':
	[
		[
			'_type == "loadable_module"',
			{
				'ldflags':
				[
					'<(android_lib_path)',
					'-lstdc++',
					'-landroid',
				],

			},
		],
		[
			'silence_warnings == 0',
			{
				'cflags':
				[
					'-Wall',
					'-Wextra',
					'-Wno-unused-parameter',	# Just contributes build noise
					'-Werror=conversion-null',
				],
				
				'cflags_c':
				[
					'-Werror=declaration-after-statement',	# Ensure compliance with C89
				],

				'cflags_cc':
				[
					'-Werror=delete-non-virtual-dtor',
					'-Werror=overloaded-virtual',
				]
			},
			{
				'cflags':
				[
					'-w',						# Disable warnings
					'-fpermissive',				# Be more lax with old code
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
				'-O0',
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
	
	'defines':
	[
		'TARGET_PLATFORM_MOBILE',
		'TARGET_SUBPLATFORM_ANDROID',
		'ANDROID',
		'_MOBILE',
		'ANDROID_NDK',
	],
}
