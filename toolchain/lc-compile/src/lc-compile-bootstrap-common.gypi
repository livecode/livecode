{
	'variables':
	{
		'silence_warnings': 1,
	},

	'direct_dependent_settings':
	{
		'variables':
		{
			'current_lc-compile': '<(PRODUCT_DIR)/<(_product_name)',
		},
	},
	
	'sources':
	[
		# Some build systems require at least one input file
		'dummy.cpp',
	],
	
	'actions':
	[
		{
			'action_name': 'bootstrap',
			'message': 'Generating full lc-compile grammar',
			
			'inputs':
			[
				'>(template_grammar_file)',
				'>@(all_syntax_files)',
			],
			
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/>(stage)/grammar_full.g',
                
                # A specific output file is required here to ensure that all
                # build systems create the output directory while also
                # also preventing spurious rebuilds.
                '<(INTERMEDIATE_DIR)/>(stage)/modules/lci/com.livecode.type.lci',
			],
			
			'action':
			[
				'>(current_lc-compile)',
				'--bootstrap',
				'--inputg', '>(template_grammar_file)',
				'--outputi', '<(INTERMEDIATE_DIR)/>(stage)/modules/lci',
				'--outputg', '<(INTERMEDIATE_DIR)/>(stage)/grammar_full.g',
				'>@(all_syntax_files)',
			],
		},
		{
			'action_name': 'gentle',
			'message': 'Running gentle on full lc-compile grammar',
			'process_outputs_as_sources': 1,
			
			'inputs':
			[
				'<(INTERMEDIATE_DIR)/>(stage)/grammar_full.g',
			],
			
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/>(stage)/gen.lit',
				'<(INTERMEDIATE_DIR)/>(stage)/gen.tkn',
				'<(INTERMEDIATE_DIR)/>(stage)/grammar_full.c',
				'<(INTERMEDIATE_DIR)/>(stage)/gen.h',
				'<(INTERMEDIATE_DIR)/>(stage)/gen.y',
			],
			
			'action':
			[
				'>(gentle_exe_file)',
				'-inputdir', '.',
				'gen.lit=<(INTERMEDIATE_DIR)/>(stage)/gen.lit',
				'gen.tkn=<(INTERMEDIATE_DIR)/>(stage)/gen.tkn',
				'grammar_full.c=<(INTERMEDIATE_DIR)/>(stage)/grammar_full.c',
				'gen.h=<(INTERMEDIATE_DIR)/>(stage)/gen.h',
				'gen.y=<(INTERMEDIATE_DIR)/>(stage)/gen.y',
				'<(INTERMEDIATE_DIR)/>(stage)/grammar_full.g',
			],
		},
		{
			'action_name': 'reflex',
			'message': 'Running reflex on full lc-compile grammar',
			'process_outputs_as_sources': 1,
			
			'inputs':
			[
				'>@(reflex_source_files)',
				'<(INTERMEDIATE_DIR)/>(stage)/gen.lit',
				'<(INTERMEDIATE_DIR)/>(stage)/gen.tkn',
			],
			
			'outputs':
			[
				'<(INTERMEDIATE_DIR)/>(stage)/gen.l',
			],
			
			'action':
			[
				'>(reflex_exe_file)',
				'>@(reflex_source_files)',
				'gen.lit=<(INTERMEDIATE_DIR)/>(stage)/gen.lit',
				'gen.tkn=<(INTERMEDIATE_DIR)/>(stage)/gen.tkn',
				'gen.l=<(INTERMEDIATE_DIR)/>(stage)/gen.l',
			],
		},
	],
}
