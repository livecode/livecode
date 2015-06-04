{
	'includes':
	[
		'../../../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'gentle',
			'type': 'executable',
			
			'toolsets': ['host','target'],
			
			'product_name': 'gentle-<(_toolset)',
			
			'dependencies':
			[ 
				'./grts.gyp:grts'
			],
			
			'direct_dependent_settings':
			{
				'variables':
				{
					'gentle_exe_file': '<(PRODUCT_DIR)/<(_product_name)<(EXECUTABLE_SUFFIX)',
				},
			},
			
			'sources':
			[
				'cyfront.c',
				'input.c',
				'main.c',
				'msg.c',
				'output.c',
				'symtab.c',
				'yytab.c',
			],
		},
	],
}

