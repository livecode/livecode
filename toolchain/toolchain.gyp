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
				'lc-compile/lc-run.gyp:lc-run',
			],
		},
	],
}
