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
			
			#'toolsets': [ '<(host_only)' ],
			
			'sources':
			[
				'reflex.c',
			],
		},
	],
}

