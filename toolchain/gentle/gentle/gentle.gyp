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
			
			#'toolsets': [ '<(host_only)' ],
			
			'dependencies':
			[ 
				'./grts.gyp:grts'
			],
			
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

