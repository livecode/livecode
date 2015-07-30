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
		
			'variables':
			{
				'silence_warnings': 1,
			},
	
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
			
			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
					
					# On Windows, gentle can run out of stack space in certain
					# builds (it needs more than the 1MB default provided).
					# Increase the number to 64MB to be on the safe side
					'StackReserveSize': '0x04000000',
				},
			},
		},
	],
}

