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
							'src/lc-compile-bootstrap.gyp:lc-compile-stage4#target',
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
			
			'direct_dependent_settings':
			{
				'variables':
				{
					'dist_files':
					[
						'<(lc-compile_target)',
					],
				},
			},
		},
	],
}
