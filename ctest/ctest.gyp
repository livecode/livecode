{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libctest',
			'type': 'static_library',

			'variables':
			{
				'library_for_module': 1,
			},

			'include_dirs':
			[
				'googletest/googletest/include',
				'googletest/googletest',
			],
			
			'sources':
			[
				'googletest/googletest/src/gtest-all.cc',
				'googletest/googletest/src/gtest_main.cc',
			],

			'defines':
			[
				'GTEST_HAS_POSIX_RE=0',
				'GTEST_HAS_PTHREAD=0',
			],


			'all_dependent_settings':
			{
				'defines':
				[
					'GTEST_HAS_POSIX_RE=0',
					'GTEST_HAS_PTHREAD=0',
				],

				'include_dirs':
				[
					'googletest/googletest/include',
				],
			},
		},
	],
}