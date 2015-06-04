{
	'cflags':
	[
		'-Wall',
		'-Wextra',
		'-fstrict-aliasing',
	],
	
	'cflags_c':
	[
		'-std=gnu99',
		'-Wstrict-prototypes',
	],
	
	'cflags_cc':
	[
		'-std=c++03',
		'-fno-exceptions',
		'-fno-rtti',
	],
	
	'configurations':
	{
		'Debug':
		{
			'cflags':
			[
				'-Og',
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
