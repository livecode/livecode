{		
	'dependencies':
	[
		'encode_version',
		
		'../libcore/libcore.gyp:libCore',
		'../libgraphics/libgraphics.gyp:libGraphics',
		'../util/perfect/perfect.gyp:perfect',
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
