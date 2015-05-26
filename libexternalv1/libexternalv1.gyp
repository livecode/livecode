{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libExternalV1',
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
				'include/external.h',
				
				'src/external.c',
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
