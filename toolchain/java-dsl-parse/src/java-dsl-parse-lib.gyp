{
	'includes':
	[
		'../../../common.gypi',
		'java-dsl-parse-sources.gypi'
	],

	'targets':
	[
		{
			'target_name': 'java-dsl-parse-lib',
			'type': 'static_library',

			'dependencies':
			[
				'../../libcompile/libcompile.gyp:libcompile',
			],
	
			'toolsets': ['host', 'target'],

			'product_name': 'java-dsl-parse-lib-<(_toolset)',

			'sources':
			[
					'<@(java-dsl-parse_source_files)',
			],
		},
	],
}
