{
	'variables':
	{
		'module_name': 'kernel-standalone',
		'module_test_dependencies':
		[
			'kernel-standalone',
			'engine-common.gyp:security-community',
			'../libfoundation/libfoundation.gyp:libFoundation',
			'../libgraphics/libgraphics.gyp:libGraphics',
		],
		'module_test_include_dirs':
		[
			'include',
			'src',
		],
		'module_test_defines': [ 'MODE_STANDALONE', ],
		'module_test_sources':
		[
			'<@(engine_test_source_files)',
		],
	},

	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
		'../config/cpptest.gypi'
	],

	'targets':
	[
		{
			'target_name': 'kernel-standalone',
			'type': 'static_library',
			
			'dependencies':
			[
				'kernel.gyp:kernel',
			],
			
			'includes':
			[
				'kernel-mode-template.gypi',
			],
			
			'variables':
			{
				'mode_macro': 'MODE_STANDALONE',
			},
			
			'sources':
			[
				'<@(engine_standalone_mode_source_files)',
			],
			
			# Standalones do *not* contain error message strings
			'sources!':
			[
				'<(INTERMEDIATE_DIR)/src/encodederrors.cpp',
			],
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'dependencies':
						[
							'kernel.gyp:kernel-java',
						],
						
						'sources':
						[
							'src/mblandroidad.cpp',
						],
						
						'actions':
						[
							{
								'action_name': 'jar',
								'message': 'JAR',
						
								'inputs':
								[
									# Depend on the Java source files directly to ensure correct updates
									'<@(engine_aidl_source_files)',
									'<@(engine_java_source_files)',
								],
					
								'outputs':
								[
									'<(PRODUCT_DIR)/Classes-Community',
								],
					
								'action':
								[
									'<(jar_path)',
									'cf',
									'<@(_outputs)',
									'-C', '<(PRODUCT_DIR)/classes_livecode_community',
									'.',
								],
							},
						],
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files': [ '<(PRODUCT_DIR)/Classes-Community' ],
							},
						},
					},
				],
			],
		},
	],
}
