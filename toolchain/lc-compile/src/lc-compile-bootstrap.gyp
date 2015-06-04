{
	'includes':
	[
		'../../../common.gypi',
	],
	
	'target_defaults':
	{
		'includes':
		[
			'lc-compile-sources.gypi',
		],
		
		'dependencies':
		[
			'../../../libfoundation/libfoundation.gyp:libFoundation',
			'../../../libscript/libscript.gyp:libScript',
			'../../gentle/gentle/gentle.gyp:gentle',
			'../../gentle/gentle/grts.gyp:grts',
			'../../gentle/reflex/reflex.gyp:reflex',
		],
		
		'include_dirs':
		[
			'.',
		],
		
		'sources':
		[
			'>@(gentle_auxiliary_grammar_files)',
			
			# Force Gyp to run the flex/bison rules
			'<(INTERMEDIATE_DIR)/>(stage)/gen.l',
			'<(INTERMEDIATE_DIR)/>(stage)/gen.y',
		],
		
		'conditions':
		[
			[
				'OS == "win"',
				{
					# MSVC doesn't recognise "inline" in C mode
					'defines':
					[
						['inline', '_inline'],
					],
				},
				{
					# Use an old C standard and disable warnings as these files
					# are machine-generated
					'cflags_c':
					[
						'-w', '-Wno-error', '-Wno-return-type', '-std=gnu89',
					],
					
					'xcode_settings':
					{
						'OTHER_CFLAGS': [ '-W', '-Wno-error', '-std=gnu89' ],
					},
				},
			],
		],
		
		'rules':
		[
			{
				'rule_name': 'flex',
				'extension': 'l',
				'process_outputs_as_sources': 1,
		
				'inputs': [],
		
				'outputs':
				[
					'<(INTERMEDIATE_DIR)/>(stage)/<(RULE_INPUT_ROOT).l.c',
				],
		
				'message': 'Lexing "<(RULE_INPUT_NAME)"',
		
				'action':
				[
					'<(flex)',
					'-o', '<@(_outputs)',
					'-w',					# Ignore warnings
					'<(RULE_INPUT_PATH)',
				],
			},
			{
				'rule_name': 'bison',
				'extension': 'y',
				'process_outputs_as_sources': 1,
		
				'inputs': [],
		
				'outputs':
				[
					'<(INTERMEDIATE_DIR)/>(stage)/<(RULE_INPUT_ROOT).y.c',
				],
		
				'message': 'Yaccing "<(RULE_INPUT_NAME)"',
		
				'action':
				[
					'<(bison)',
					'-o', '<@(_outputs)',
					'-Wnone',				# Ignore warnings
					'<(RULE_INPUT_PATH)',
				],
			},
			{
				'rule_name': 'auxiliary_grammar',
				'extension': 'g',
				'process_outputs_as_sources': 1,
				
				'inputs':
				[
					'<(INTERMEDIATE_DIR)/>(stage)/gen.lit',
					'<(INTERMEDIATE_DIR)/>(stage)/gen.tkn',
				],
				
				'outputs':
				[
					'<(INTERMEDIATE_DIR)/>(stage)/<(RULE_INPUT_ROOT).c',
				],
				
				'message': 'Running gentle on auxiliary grammar "<(RULE_INPUT_NAME)"',
				
				'action':
				[
					'>(gentle_exe_file)',
					'<(RULE_INPUT_PATH)',
					'<(RULE_INPUT_ROOT).c=<(INTERMEDIATE_DIR)/>(stage)/<(RULE_INPUT_ROOT).c',
				],
			},
		],
	},
	
	'targets':
	[
		{
			'target_name': 'lc-bootstrap-compile',
			'type': 'executable',
			
			'toolsets': ['host','target'],

			'product_name': 'lc-bootstrap-compile-<(_toolset)',
			
			'variables':
			{
				'stage': 'stage1',
			},
			
			'direct_dependent_settings':
			{
				'variables':
				{
					'current_lc-compile': '<(PRODUCT_DIR)/<(_product_name)<(EXECUTABLE_SUFFIX)',
				},
			},
			
			'defines':
			[
				'BOOTSTRAP',
			],
			
			'sources':
			[
				'>@(lc-compile_source_files)',
			],
			
			'actions':
			[
				{
					'action_name': 'gentle',
					'message': 'Running gentle on bootstrap grammar',
					'process_outputs_as_sources': 1,
					
					'inputs':
					[ 
						'>(gentle_bootstrap_grammar_file)',
						'>@(gentle_auxiliary_grammar_files)',
					],
					
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/<(stage)/gen.lit',
						'<(INTERMEDIATE_DIR)/<(stage)/gen.tkn',
						'<(INTERMEDIATE_DIR)/<(stage)/grammar.c',
						'<(INTERMEDIATE_DIR)/<(stage)/gen.h',
						'<(INTERMEDIATE_DIR)/<(stage)/gen.y',
					],
					
					'action':
					[
						'>(gentle_exe_file)',
						'>(gentle_bootstrap_grammar_file)',
						'gen.lit=<(INTERMEDIATE_DIR)/<(stage)/gen.lit',
						'gen.tkn=<(INTERMEDIATE_DIR)/<(stage)/gen.tkn',
						'grammar.c=<(INTERMEDIATE_DIR)/<(stage)/grammar.c',
						'gen.h=<(INTERMEDIATE_DIR)/<(stage)/gen.h',
						'gen.y=<(INTERMEDIATE_DIR)/<(stage)/gen.y',
					],
				},
				{
					'action_name': 'reflex',
					'message': 'Running reflex on bootstrap grammar',
					'process_outputs_as_sources': 1,
					
					'inputs':
					[
						'>@(reflex_source_files)',
						'<(INTERMEDIATE_DIR)/<(stage)/gen.lit',
						'<(INTERMEDIATE_DIR)/<(stage)/gen.tkn',
					],
					
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/<(stage)/gen.l',
					],
					
					'action':
					[
						'>(reflex_exe_file)',
						'>@(reflex_source_files)',
						'gen.lit=<(INTERMEDIATE_DIR)/<(stage)/gen.lit',
						'gen.tkn=<(INTERMEDIATE_DIR)/<(stage)/gen.tkn',
						'gen.l=<(INTERMEDIATE_DIR)/<(stage)/gen.l',
					],
				},
			],
		},
		{
			'target_name': 'lc-compile-stage2',
			'type': 'executable',
			
			'toolsets': ['host','target'],
			
			'product_name': 'lc-compile-stage2-<(_toolset)',
			
			'variables':
			{
				'stage': 'stage2',
			},
			
			'dependencies':
			[
				'lc-bootstrap-compile',
			],
			
			'includes':
			[
				'lc-compile-bootstrap-common.gypi',
			],
		},
	],
}
