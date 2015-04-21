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
					[
						# FIXME Force the perfect executable to be put into
						# the target SDK's output directory, so that it
						# appears in the PRODUCT_DIR when building against
						# the target SDK
						'OS == "ios"',
						{
							'SYMROOT': '$(SOLUTION_DIR)/_build/ios/<(target_sdk)',
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
