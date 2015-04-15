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
		},
	],
	
	'variables':
	{
		'conditions':
		[
			[
				'host_os == "win"',
				{
					'perfect_name': '<(PRODUCT_DIR)/perfect.exe',
				},
			],
			[
				'host_os == "linux" or host_os == "mac"',
				{
					'perfect_name': '<(PRODUCT_DIR)/perfect',
				},
			],
		],
	}
}
