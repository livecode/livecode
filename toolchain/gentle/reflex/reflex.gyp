{
	'includes':
	[
		'../../../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'reflex',
			'type': 'executable',
			
			'toolsets': ['host','target'],
			
			'product_name': 'reflex-<(_toolset)',
			
			'direct_dependent_settings':
			{
				'variables':
				{
					'reflex_exe_file': '<(PRODUCT_DIR)/<(_product_name)<(EXECUTABLE_SUFFIX)',
				},
			},
			
			'sources':
			[
				'reflex.c',
			],
		},
	],
}

