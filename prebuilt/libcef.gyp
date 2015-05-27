{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libcef',
			'type': 'none',
			
			'direct_dependent_settings':
			{
				'defines':
				[
					'USING_CEF_SHARED=1',
				],
			},
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'link_settings':
						{
							'libraries':
							[
								'lib/libcef/mac/Chromium Embedded Framework.framework',
							],
							
							'xcode_settings':
							{
								'FRAMEWORK_SEARCH_PATHS': '$(SOLUTION_DIR)/prebuilt/lib/mac',
							},
						},
					},
				],
			],
		},
	],
}
