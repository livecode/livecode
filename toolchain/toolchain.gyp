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
				'lc-compile-ffi-java/lc-compile-ffi-java.gyp:lc-compile-ffi-java',
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
