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

			'toolsets': ['host', 'target'],

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
				'include/atlsubset.h',
				'include/core.h',
				'include/filesystem.h',
				'include/thread.h',
				'include/thunk.h',
				
				'src/core.cpp',
				'src/filesystem.cpp',
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
