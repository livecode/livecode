{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'external-revandroid',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revandroid',
			
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
					'OS == "linux"',
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
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		},
		
		{
			'target_name': 'external-reviphone',
			'type': 'none',
			'product_name': 'reviphone',
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'type': 'loadable_module',
						'mac_bundle': 1,
						
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
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
							},
						},
					},
				],
			],
			
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

			'xcode_settings':
			{
				'INFOPLIST_FILE': 'reviphone-Info.plist',
				'CLANG_LINK_OBJC_RUNTIME': 'YES',
				'SDKROOT': 'macosx',
			},
		},
		
		{
			'target_name': 'reviphoneproxy',
			'type': 'none',

			'conditions':
			[
				[
					'OS == "mac"',
					{
						'type': 'executable',

						'xcode_settings':
						{
							'ARCHS': 'x86_64',
							'SDKROOT': 'macosx',
						},
					},
				],
			],
			
			'sources':
			[
				'src/reviphoneproxy.mm',
			],
			
			'libraries':
			[
				'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
			],

			'xcode_settings':
			{
				'CLANG_LINK_OBJC_RUNTIME': 'YES',
			},
		},
	],
}
