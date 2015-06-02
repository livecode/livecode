{
	'targets':
	[
		{
			'target_name': 'kernel-standalone',
			'type': 'static_library',
			
			'dependencies':
			[
				'kernel',
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
							'kernel-java',
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
