{
	'include':
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
			],
		},
	],
}
