{
	'includes':
	[
		'../../../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'grts',
			'type': 'static_library',
			
			'toolsets': ['host','target'],
			
			'suppress_warnings': 1,
			
			'sources':
			[
				'grts.c',
			],
		},
	],
}
