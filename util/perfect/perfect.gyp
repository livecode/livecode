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

			'toolsets': ['target','host'],

			'product_name': 'perfect-<(_toolset)',

			'sources':
			[
				'perfect.c',
			],

            'variables':
            {
                'silence_warnings': 1,
            },

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
						},
					],
                    [
                        'OS == "mac" and target_sdk != "macosx10.6"',
                        {
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
                            'ARCHS': '<(host_arch)',
						},
					],
				],
			},

			'direct_dependent_settings':
			{
				'variables':
				{
					'perfect_path': '<(PRODUCT_DIR)/<(_product_name)',
				},
			},
			
			'conditions':
			[
				[
					'OS == "ios"',
					{
						'direct_dependent_settings':
						{
							'variables':
							{
								'perfect_path': '<(mac_tools_dir)/perfect-host',
							},
						},
					},
				],
			],
		},
	],
}
