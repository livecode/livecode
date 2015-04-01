{		
	'dependencies':
	[
		'encode_version',
	
		'../libcore/libcore.gyp:libCore',
		'../libgraphics/libgraphics.gyp:libGraphics',
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
				'encode_errors.rev',
				'src/executionerrors.h',
				'src/parseerrors.h',
			],
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/src/encodederrors.cpp',
			],
		
			'action':
			[
				'<(revolution_path)',
				'encode_errors.rev',
				'./src',
				'<@(_outputs)',
			],
		},
		{
			'action_name': 'Build keyword hash table',
			
			'inputs':
			[
				'hash_strings.rev',
				
				# It really does depend on this but gyp doesn't compile sources listed as action inputs...
				#'src/lextable.cpp',
			],
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/src/hashedstrings.cpp',
			],
		
			'action':
			[
				'<(revolution_path)',
				'hash_strings.rev',
				'./src/lextable.cpp',
				'<@(_outputs)',
				'<(perfect_path)',
			],
		},
	],
}
