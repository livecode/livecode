{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revandroid',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternalv1/libexternalv1.gyp:libExternalV1',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/revandroid.cpp',
			],
			
			'conditions':
			[
				[
					'OS == "linux" or OS =="mac"',
					{
						'libraries':
						[
							'-lpthread',
						],
					}
				],
				[
					'OS == "win"',
					{
						'libraries':
						[
							'-lWS2_32',
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'revandroid-Info.plist',
			},
		},
		
		{
			'target_name': 'reviphone',
			'type': '<(target_type)',
			'mac_bundle': 1,
			
			'variables':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
							'target_type': 'loadable_module',
						},
						{
							'target_type': 'none',
						},
					]
				],
			},
			
			'dependencies':
			[
				'../libexternalv1/libexternalv1.gyp:libExternalV1',
				'reviphoneproxy',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/CoreSimulator.h',
				'src/DVTiPhoneSimulatorRemoteClient.h',
				'src/reviphone.mm',
			],
			
			'libraries':
			[
				'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
			],
			
			'conditions':
			[
				[
					# reviphone is only usable on OSX
					'OS == "mac"',
					{
						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)/reviphone.bundle/Contents/MacOS',
								'files':
								[
									'<(PRODUCT_DIR)/reviphoneproxy',
								],
							},
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'reviphone-Info.plist',
			},
		},
		
		{
			'target_name': 'reviphoneproxy',
			'type': '<(target_type)',
			
			'variables':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
							'target_type': 'executable',
						},
						{
							'target_type': 'none',
						},
					],
				],
			},
			
			'sources':
			[
				'src/reviphoneproxy.mm',
			],
			
			'libraries':
			[
				'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
			],
		},
	],
}
