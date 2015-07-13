{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libcef',
			'type': 'none',
			
			'dependencies':
			[
				'fetch.gyp:fetch',
			],
			
			'direct_dependent_settings':
			{
				'defines':
				[
					'USING_CEF_SHARED=1',
				],
			},
		},
	],
}
