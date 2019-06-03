{
	'includes':
	[
		'../../common.gypi',
	],
	
	
	'targets':
	[
		{
			'target_name': 'lc-compile',
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
							'src/lc-compile-bootstrap.gyp:lc-compile-stage3#target',
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'lc-compile_target': '<(PRODUCT_DIR)/lc-compile<(EXECUTABLE_SUFFIX)',
								'lc-compile_host':   '<(PRODUCT_DIR)/lc-compile<(EXECUTABLE_SUFFIX)',
							},
						},
					},
					{
						'dependencies':
						[
							'src/lc-compile-bootstrap.gyp:lc-compile-stage3#host',
						],
						
						'conditions':
						[
							[
								'mobile == 0',
								{
									'dependencies':
									[
										'src/lc-compile-bootstrap.gyp:lc-compile-stage4#target',
									],

									'direct_dependent_settings':
									{
										'variables':
										{
											'dist_files':
											[
												'>(lc-compile_target)',
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
											'lc-compile_host': '<(mac_tools_dir)/lc-compile-stage3-host',
										},
									},
								},
							],
						],
						
						'direct_dependent_settings':
						{
							'variables':
							{
								'lc-compile_target': '<(PRODUCT_DIR)/lc-compile<(EXECUTABLE_SUFFIX)',
								'lc-compile_host':   '<(PRODUCT_DIR)/lc-compile-stage3-host<(EXECUTABLE_SUFFIX)',
							},
						},
					},
				],
			],
		},
	],
}
