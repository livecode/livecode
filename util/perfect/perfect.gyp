{
	'includes':
	[
		'../../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'perfect',
			'type': 'executable',
	
			'conditions':
			[
				[
					'OS == "linux" or OS == "android"',
					{
						'toolsets': ['host'],	
					},
				],
			],

			'sources':
			[
				'perfect.c',
			],
			
			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},
			
			'xcode_settings':
			{
				'conditions':
				[
					[
						'OS == "mac" or OS == "ios"',
						{
							'SDKROOT': '<(host_sdk)',
							'ARCHS': '<(host_arch)',
						},
					],
				],
			},

			'direct_dependent_settings':
			{
				'variables':
				{
					'perfect_path': '<(PRODUCT_DIR)/perfect<(EXECUTABLE_SUFFIX)',
				},
			},
		},
	],
}
