{
	'variables':
	{
		'module_name': 'libcpptest',
		'module_test_sources':
		[
			'test/trivial.cpp',
		],
	},

	'includes':
	[
		'../common.gypi',
		'../config/cpptest.gypi',
	],

	'targets':
	[
		{
			'target_name': 'libcpptest',
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
				'src/gtest_main.cpp',
				'src/MCTapListener.cpp',
			],

			'defines':
			[
				'GTEST_HAS_POSIX_RE=0',
				'GTEST_HAS_PTHREAD=0',
				'GTEST_HAS_RTTI=0',
				'GTEST_HAS_EXCEPTIONS=0',
			],

			'all_dependent_settings':
			{
				'defines':
				[
					'GTEST_HAS_POSIX_RE=0',
					'GTEST_HAS_PTHREAD=0',
					'GTEST_HAS_RTTI=0',
					'GTEST_HAS_EXCEPTIONS=0',
				],

				'include_dirs':
				[
					'googletest/googletest/include',
				],
			},

			'conditions':
			[
				[
					'OS == "emscripten"',
					{
						'include_dirs':
						[
							'$(EMSCRIPTEN)/system/lib/libcxxabi/include',
						],

						'all_dependent_settings':
						{
							'include_dirs':
							[
								'$(EMSCRIPTEN)/system/lib/libcxxabi/include',
							],
						},
					},
				],
			],
		},
	],
}
