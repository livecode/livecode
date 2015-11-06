{
	'includes':
	[
		'common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'LiveCode-minimal',
			'type': 'none',
			
			'dependencies':
			[
				# Engines
			],
			
			'conditions':
			[
				[
					'mobile == 0',
					{
						'dependencies':
						[
							# Engines
                            'engine/engine.gyp:server',
                            'engine/engine.gyp:profiling-server',
						],
					},
				],
			],
		},
    ],
}
