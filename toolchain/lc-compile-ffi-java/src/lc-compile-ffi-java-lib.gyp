{
	'includes':
	[
		'../../../common.gypi',
		'lc-compile-ffi-java-sources.gypi'
	],

	'targets':
	[
		{
			'target_name': 'lc-compile-ffi-java-lib',
			'type': 'static_library',

			'dependencies':
			[
				'../../libcompile/libcompile.gyp:libcompile',
			],
	
			'toolsets': ['host', 'target'],

			'product_name': 'lc-compile-ffi-java-lib-<(_toolset)',

			'sources':
			[
					'<@(lc-compile-ffi-java_source_files)',
			],
		},
	],
}
