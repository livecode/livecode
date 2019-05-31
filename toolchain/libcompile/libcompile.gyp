{
	'includes':
	[
		'../../common.gypi',
	],

	'targets':
	[
		{
			'target_name': 'libcompile',
			'type': 'static_library',

			'toolsets': ['host', 'target'],

			'product_name': 'libcompile-<(_toolset)',

			'dependencies':
			[
				'../../libfoundation/libfoundation.gyp:libFoundation',
				'../../libscript/libscript.gyp:libScript',
				'../../prebuilt/thirdparty.gyp:thirdparty_prebuilt_ffi',
			],

			'sources':
			[
				'include/allocate.h',
				'include/literal.h',
				'include/output.h',
				'include/position.h',
				'include/report.h',
				
				'src/allocate.cpp',
				'src/literal.c',
				'src/output.cpp',
				'src/position.c',
				'src/report.c',
			],

			'include_dirs':
			[
				'include',
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
		},
	],
}
