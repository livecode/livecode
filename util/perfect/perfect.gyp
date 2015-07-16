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

			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
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
