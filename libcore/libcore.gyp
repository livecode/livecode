{
	# ----- LibCore -----
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libCore',
			'type': 'static_library',

			'variables':
			{
				'library_for_module': 1,
			},

			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'src/binary.cpp',
				'src/core.cpp',
				'src/filesystem.cpp',
				'src/module.cpp',
				'src/sserialize.cpp',
				'src/sserialize_lnx.cpp',
				'src/sserialize_osx.cpp',
				'src/sserialize_w32.cpp',
				'src/thread.cpp',
				'src/thunk.cpp',
			],
			
			'conditions':
			[
				[
					'OS != "win"',
					{
						'sources!':
						[
							'src/thunk.cpp',
						],
					},
				],
				[
					'OS == "win"',
					{
						'link_settings':
						{
							'libraries':
							[
								'-loleaut32',
							],
						},
					},
				],
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
		},
	],
}