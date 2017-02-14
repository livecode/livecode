{
	'includes':
	[
		'../../../common.gypi',
	],
	
	'target_defaults':
	{
		'includes':
		[
			'lc-compile-ffi-java-sources.gypi',
		],

		'dependencies':
		[
			'../../gentle/gentle/grts.gyp:grts',
			'../../libcompile/libcompile.gyp:libcompile',
			'lc-compile-ffi-java-lib.gyp:lc-compile-ffi-java-lib',
		],
		
		'include_dirs':
		[
			'.',
		],
		
		'sources':
		[
			'>@(gentle_auxiliary_grammar_files)',
			
			# Force Gyp to run the flex/bison rules
			'<(INTERMEDIATE_DIR)/gen.l',
			'<(INTERMEDIATE_DIR)/gen.y',
		],
		
		'conditions':
		[
			[
				'OS == "win"',
				{
					# MSVC doesn't recognise "inline" in C mode and has no
					# <unistd.h> header
					'defines':
					[
						['inline', '_inline'],
						['YY_NO_UNISTD_H', '1'],
					],
					
					'msvs_settings':
					{
						'VCLinkerTool':
						{
							'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
							
							# On Windows, lc-compile-ffi-java can run out of stack space in certain
							# builds (it needs more than the 1MB default provided).
							# Increase the number to 64MB to be on the safe side
							'StackReserveSize': '0x04000000',
						},
					},
				},
				{
					# Use an old C standardare machine-generated
					'cflags_c':
					[
						'-std=gnu89',
					],
					
					'xcode_settings':
					{
						'OTHER_CFLAGS': [ '-std=gnu89' ],
						'OTHER_CPLUSPLUSFLAGS': [],		# Suppress the default "$(OTHER_CFLAGS)"
					},
				},
			],
			[
				'cross_compile != 0',
				{
					'dependencies':
					[
						'../../gentle/gentle/gentle.gyp:gentle#host',
						'../../gentle/reflex/reflex.gyp:reflex#host',
					],
				},
				{
					'dependencies':
					[
						'../../gentle/gentle/gentle.gyp:gentle#target',
						'../../gentle/reflex/reflex.gyp:reflex#target',
					],
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
					'<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).l.c',
				],
		
				'message': 'Lexing "<(RULE_INPUT_NAME)"',
		
				'action':
				[
					'<@(flex)',
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
					'<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).y.c',
				],
		
				'message': 'Yaccing "<(RULE_INPUT_NAME)"',
		
				'action':
				[
					'<@(bison)',
					'-o', '<@(_outputs)',
					#'-Wnone',				# Ignore warnings -- disabled as incompatible with XCode bison
					'<(RULE_INPUT_PATH)',
				],
			},
			{
				'rule_name': 'auxiliary_grammar',
				'extension': 'g',
				'process_outputs_as_sources': 1,
				
				'inputs':
				[
					'<(INTERMEDIATE_DIR)/gen.lit',
					'<(INTERMEDIATE_DIR)/gen.tkn',
				],
				
				'outputs':
				[
					'<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c',
				],
				
				'message': 'Running gentle on auxiliary grammar "<(RULE_INPUT_NAME)"',
				
				'action':
				[
					'>(gentle_exe_file)',
					'-inputdir', '.',
					'<(RULE_INPUT_ROOT).c=<(INTERMEDIATE_DIR)/<(RULE_INPUT_ROOT).c',
					'<(RULE_INPUT_PATH)',
				],
			},
		],
	},
	
	'targets':
	[
		{
			'target_name': 'lc-compile-ffi-java',
			'type': 'executable',
			
			'toolsets': ['target', 'host'],
		
			'variables':
			{
				'silence_warnings': 1,
			},

			'sources':
			[
				# Some build systems require at least one input file
				'dummy.cpp',
			],

			'conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'lc-compile-ffi-java-<(_toolset)',
					},
					{
						'product_name': 'lc-compile-ffi-java',
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files':
								[
									'<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)',
								],
							},
						},
					},
				],
			],

			'actions':
			[
				{
					'action_name': 'gentle',
					'message': 'Running gentle on grammar',
					'process_outputs_as_sources': 1,
					
					'inputs':
					[ 
						'>(gentle_grammar_file)',
						#'>@(gentle_auxiliary_grammar_files)',	# Confuses Gyp/MSVS when listed here
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
						'>(gentle_grammar_file)',
						'gen.lit=<(INTERMEDIATE_DIR)/gen.lit',
						'gen.tkn=<(INTERMEDIATE_DIR)/gen.tkn',
						'grammar.c=<(INTERMEDIATE_DIR)/grammar.c',
						'gen.h=<(INTERMEDIATE_DIR)/gen.h',
						'gen.y=<(INTERMEDIATE_DIR)/gen.y',
					],
				},
				{
					'action_name': 'reflex',
					'message': 'Running reflex on grammar',
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
						'gen.l=<(INTERMEDIATE_DIR)/gen.l',
					],
				},
			],
		},
	],
}
