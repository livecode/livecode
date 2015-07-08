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
			
			'product_name': 'grts',
			
			'sources':
			[
				'grts.c',
			],
			
			'target_conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'grts->(_toolset)',
					},
				],
			],
		},
	],
}
