{
	'includes':
	[
		'../../common.gypi',
	],
	
	
	'targets':
	[
		{
			'target_name': 'java-dsl-parse',
			'type': 'none',
			
			'toolsets': ['host','target'],
			
			'conditions':
			[
				[
					'cross_compile == 0',
					{
						'dependencies':
						[
							'src/java-dsl-parse-generate.gyp:java-dsl-parse#target',
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'java-dsl-parse_target': '<(PRODUCT_DIR)/java-dsl-parse<(EXECUTABLE_SUFFIX)',
								'java-dsl-parse_host':   '<(PRODUCT_DIR)/java-dsl-parse<(EXECUTABLE_SUFFIX)',
							},
						},
					},
					{
						'dependencies':
						[
							'src/java-dsl-parse-generate.gyp:java-dsl-parse#host',
						],
						
						'conditions':
						[
							[
								'mobile == 0',
								{
									'dependencies':
									[
										'src/java-dsl-parse-generate.gyp:java-dsl-parse#target',
									],

									'direct_dependent_settings':
									{
										'variables':
										{
											'dist_files':
											[
												'<(java-dsl-parse_target)',
											],
										},
									},
								},
							],
							[
								'OS == "ios"',
								{
									'direct_dependent_settings':
									{
										'variables':
										{
											# Work-around for Xcode/Gyp impedance mismatch
											'java-dsl-parse_host': '<(mac_tools_dir)/java-dsl-parse-host',
										},
									},
								},
							],
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'java-dsl-parse_target': '<(PRODUCT_DIR)/java-dsl-parse-target<(EXECUTABLE_SUFFIX)',
								'java-dsl-parse_host':   '<(PRODUCT_DIR)/java-dsl-parse-host<(EXECUTABLE_SUFFIX)',
							},
						},
					},
				],
			],
		},
	],
}
