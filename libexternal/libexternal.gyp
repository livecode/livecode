{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libExternal',
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
				'include/revolution/external.h',
				'include/revolution/support.h',
				
				'src/external.c',
				'src/osxsupport.cpp',
				'src/unxsupport.cpp',
				'src/w32support.cpp',
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
			
			'link_settings':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
					
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/CoreServices.framework',
							],
						},
					],
				],
			}
		},
	],
}