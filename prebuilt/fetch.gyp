{
	'includes':
	[
		'../common.gypi',
	],

	'targets':
	[
		{
			'target_name': 'fetch-all',
			'type': 'none',
			
			'dependencies':
			[
				'fetch-android',
				'fetch-linux',
				'fetch-mac',
				'fetch-win',
				'fetch-ios',
			],
		},
		{
			'target_name': 'fetch',
			'type': 'none',
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'dependencies':
						[
							'fetch-android',
						],
					},
				],
				[
					'OS == "linux"',
					{
						'dependencies':
						[
							'fetch-linux',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'dependencies':
						[
							'fetch-mac',
						],
					},
				],
				[
					'OS == "win"',
					{
						'dependencies':
						[
							'fetch-win',
						],
					},
				],
				[
					'OS == "ios"',
					{
						'dependencies':
						[
							'fetch-ios',
						],
					},
				],
			],
		},
		{
			'target_name': 'fetch-android',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Android',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/android/armv6',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'android',
					],
				},
			],
		},
		{
			'target_name': 'fetch-linux',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Linux',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/linux',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'linux',
					],
				},
			],
		},
		{
			'target_name': 'fetch-mac',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for OSX',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'mac',
					],
				},
			],
		},
		{
			'target_name': 'fetch-win',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Windows',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/win32/i386',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'win32',
					],
				},
			],
		},
		{
			'target_name': 'fetch-ios',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for iOS',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/ios',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'ios',
					],
				},
			],
		},
	],
}
