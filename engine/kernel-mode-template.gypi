{		
	'dependencies':
	[
		'engine-common.gyp:encode_version',
		
		'../libfoundation/libfoundation.gyp:libFoundation',
		'../libgraphics/libgraphics.gyp:libGraphics',
		'../libscript/libscript.gyp:libScript',

		'../libbrowser/libbrowser.gyp:libbrowser',
	],

	'conditions':
	[
		[
			'cross_compile != 0',
			{
				'dependencies':
				[
					'../util/perfect/perfect.gyp:perfect#host',
				],
			},
			{
				'dependencies':
				[
					'../util/perfect/perfect.gyp:perfect',
				],
			},
		],
	],

	'include_dirs':
	[
		'<(INTERMEDIATE_DIR)/src',
	],

	'sources':
	[
		'<@(engine_mode_dependent_files)',
	],

	'defines':
	[
		'>(mode_macro)',
	],

	'actions':
	[
		{
			'action_name': 'Encode errors',
			
			'inputs':
			[
				'../util/encode_errors.pl',
				'src/executionerrors.h',
				'src/parseerrors.h',
			],
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/src/encodederrors.cpp',
			],
		
			'action':
			[
				'<@(perl)',
				'../util/encode_errors.pl',
				'./src',
				'<@(_outputs)',
			],
		},
		{
			'action_name': 'Build keyword hash table',
			
			'inputs':
			[
				'../util/hash_strings.pl',
				
				# It really does depend on this but gyp doesn't compile sources listed as action inputs...
				#'src/lextable.cpp',
			],
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/src/hashedstrings.cpp',
			],
		
			'action':
			[
				'<@(perl)',
				'../util/hash_strings.pl',
				'./src/lextable.cpp',
				'<@(_outputs)',
				'>(perfect_path)',
			],
		},
	],
}
