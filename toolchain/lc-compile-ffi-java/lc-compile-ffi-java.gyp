{
	'includes':
	[
		'../../common.gypi',
	],
	
	
	'targets':
	[
		{
			'target_name': 'lc-compile-ffi-java',
			'type': 'none',
			
			'toolsets': ['host','target'],
			
			'dependencies':
			[
				'../../prebuilt/thirdparty.gyp:thirdparty_prebuilt_ffi',
			],

			'conditions':
			[
				[
					'cross_compile == 0',
					{
						'dependencies':
						[
							'src/lc-compile-ffi-java-generate.gyp:lc-compile-ffi-java#target',
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'lc-compile-ffi-java_target': '<(PRODUCT_DIR)/lc-compile-ffi-java<(EXECUTABLE_SUFFIX)',
								'lc-compile-ffi-java_host':   '<(PRODUCT_DIR)/lc-compile-ffi-java<(EXECUTABLE_SUFFIX)',
							},
						},
					},
					{
						'dependencies':
						[
							'src/lc-compile-ffi-java-generate.gyp:lc-compile-ffi-java#host',
						],
						
						'conditions':
						[
							[
								'mobile == 0',
								{
									'dependencies':
									[
										'src/lc-compile-ffi-java-generate.gyp:lc-compile-ffi-java#target',
									],

									'direct_dependent_settings':
									{
										'variables':
										{
											'dist_files':
											[
												'>(lc-compile-ffi-java_target)',
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
											'lc-compile-ffi-java_host': '<(mac_tools_dir)/lc-compile-ffi-java-host',
										},
									},
								},
							],
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'lc-compile-ffi-java_target': '<(PRODUCT_DIR)/lc-compile-ffi-java-target<(EXECUTABLE_SUFFIX)',
								'lc-compile-ffi-java_host':   '<(PRODUCT_DIR)/lc-compile-ffi-java-host<(EXECUTABLE_SUFFIX)',
							},
						},
					},
				],
			],
		},
	],
}
