{
	'includes':
	[
		'../../../common.gypi',
		'lc-compile-sources.gypi'
	],

	'targets':
	[
		{
			'target_name': 'lc-compile-lib',
			'type': 'static_library',

			'toolsets': ['host', 'target'],

			'product_name': 'lc-compile-lib-<(_toolset)',

			'dependencies':
			[
				'../../../libfoundation/libfoundation.gyp:libFoundation',
				'../../../libscript/libscript.gyp:libScript',
			],

			'sources':
			[
					'<@(lc-compile_source_files)',
			],
		},
	],
}
