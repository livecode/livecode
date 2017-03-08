{
	'variables':
	{
		'silence_warnings': 1,
	},

	'sources':
	[
		# Some build systems require at least one input file
		'dummy.cpp',
	],
	
	'actions':
	[
		{
			'action_name': 'gentle',
			'message': 'Running gentle on full lc-compile-ffi-java grammar',
			'process_outputs_as_sources': 1,
			
			'inputs':
			[
				'>(gentle_grammar_file)',
			],
			
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/gen.lit',
				'<(INTERMEDIATE_DIR)/gen.tkn',
				'<(INTERMEDIATE_DIR)/grammar.c',
				'<(INTERMEDIATE_DIR)/gen.h',
				'<(INTERMEDIATE_DIR)/gen.y',
			],
			
			'action':
			[
				'>(gentle_exe_file)',
				'-inputdir', '.',
				'gen.lit=<(INTERMEDIATE_DIR)/gen.lit',
				'gen.tkn=<(INTERMEDIATE_DIR)/gen.tkn',
				'grammar.c=<(INTERMEDIATE_DIR)/grammar.c',
				'gen.h=<(INTERMEDIATE_DIR)/gen.h',
				'gen.y=<(INTERMEDIATE_DIR)/gen.y',
				'>(gentle_grammar_file)',
			],
		},
		{
			'action_name': 'reflex',
			'message': 'Running reflex on full lc-compile-ffi-java grammar',
			'process_outputs_as_sources': 1,
			
			'inputs':
			[
				'>@(reflex_source_files)',
				'<(INTERMEDIATE_DIR)/gen.lit',
				'<(INTERMEDIATE_DIR)/gen.tkn',
			],
			
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/gen.l',
			],
			
			'action':
			[
				'>(reflex_exe_file)',
				'>@(reflex_source_files)',
				'gen.lit=<(INTERMEDIATE_DIR)/gen.lit',
				'gen.tkn=<(INTERMEDIATE_DIR)/gen.tkn',
				'gen.l=<(INTERMEDIATE_DIR)/>(stage)/gen.l',
			],
		},
	],
}
