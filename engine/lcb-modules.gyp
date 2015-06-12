{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',					# LCB modules for the engine are listed here
		'../libscript/stdscript-sources.gypi',	# LCB modules for the stdscript library are listed here
	],
	
	'targets':
	[
		{
			'target_name': 'engine_lcb_modules',
			'type': 'static_library',
			
			'dependencies':
			[
				'../libscript/libscript.gyp:stdscript',
				'../toolchain/lc-compile/lc-compile.gyp:lc-compile',
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files':
					[
						'<(PRODUCT_DIR)/modules',
					],
				},
			},
			
			'actions':
			[
				{
					'action_name': 'generate_builtin_list',
					'message': 'Generating list of in-engine LCB modules',
					'process_outputs_as_sources': 1,
					
					'inputs':
					[
						'<@(engine_syntax_lcb_files)',
						'<@(engine_other_lcb_files)',
						'<@(stdscript_syntax_lcb_files)',
						'<@(stdscript_other_lcb_files)',
						'>(lc-compile_host)',
					],
					
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/engine_lcb_modules.c',
						'<(PRODUCT_DIR)/modules/lci/dummy.file',
					],
					
					'action':
					[
						# We don't need the output grammar, just the modules list and interface files
						'>(lc-compile_host)',
						'--bootstrap',
						'--inputg', '../toolchain/lc-compile/src/grammar.g',
						'--outputi', '<(PRODUCT_DIR)/modules/lci',
						#'--outputg', '<(INTERMEDIATE_DIR)/>(stage)/grammar_full.g',
						'--outputc', '<(INTERMEDIATE_DIR)/engine_lcb_modules.c',
						'<@(_inputs)',
					],
				},
			],
		},	
	],
}
