{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'toolchain-all',
			'type': 'none',
			
			'dependencies':
			[
				'lc-compile/lc-compile.gyp:lc-compile',
				'java-dsl-parse/java-dsl-parse.gyp:java-dsl-parse',
			],
			
			'conditions':
			[
				[
					'mobile == 0',
					{
						'dependencies':
						[
							'lc-compile/lc-run.gyp:lc-run',
						],
					},
				],
			],
		},
	],
}
