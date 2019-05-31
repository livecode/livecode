{
	'includes':
	[
		'../../common.gypi',
		'../../libscript/stdscript-sources.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'lc-run',
			'type': 'executable',
			
			'toolsets': ['host','target'],
			
			'dependencies':
			[
				'lc-compile.gyp:lc-compile',
				'../../libfoundation/libfoundation.gyp:libFoundation',
				'../../libscript/libscript.gyp:libScript',
				'../../libscript/libscript.gyp:stdscript',
				'../../prebuilt/thirdparty.gyp:thirdparty_prebuilt_ffi',
			],
			
			'sources':
			[
				'src/lc-run.cpp',
			],
			
			'conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'lc-run-<(_toolset)',
					},
					{
						'product_name': 'lc-run',
						
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
				[
					'OS == "win"',
					{
						'libraries':
						[
							'-lshell32',
						],
					},
				],
				[
				'OS == "linux" or OS == "android"',
				{
					# Ensure that the symbols LCB binds to are exported
					'ldflags': [ '-rdynamic' ],
				},
			],
			],
			
			'actions':
			[
				{
					'action_name': 'lc-run_lcb_modules',
					'message': 'Generating list of modules for lc-run',
					'process_outputs_as_sources': 1,
					
					'inputs':
					[
						'<@(stdscript_syntax_lcb_files)',
						'<@(stdscript_other_lcb_files)',
					],
					
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/lc-run_lcb_modules.cpp',

						# A specific output file is required here to ensure that
						# all build systems create the output directory while
                        			# also preventing spurious rebuilds.
						'<(INTERMEDIATE_DIR)/lc-run-modules/lci/com.livecode.type.lci',
					],
					
					'action':
					[
						# We don't need the output grammar, just the modules list and interface files
						'>(lc-compile_host)',
						'--bootstrap',
						'--inputg', 'src/grammar.g',
						'--outputi', '<(INTERMEDIATE_DIR)/lc-run-modules/lci',
						'--outputc', '<(INTERMEDIATE_DIR)/lc-run_lcb_modules.cpp',
						'<@(_inputs)',
					],
				},
			],
			
			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},
		},
	],
}
