{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libopenssl_headers',
			'type': 'none',

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'conditions':
				[
					[
						'OS == "win"',
						{
							'include_dirs':
							[
								'unpacked/openssl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'OS != "win"',
						{
							'include_dirs':
							[
								'include',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'libopenssl',
			'type': 'none',

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'conditions':
				[
					[
						'OS == "win"',
						{
							'include_dirs':
							[
								'unpacked/openssl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'OS != "win"',
						{
							'include_dirs':
							[
								'include',
							],
						},
					],
				],
			},

			'link_settings':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
							'libraries':
							[
								'lib/mac/libcustomcrypto.a',
								'lib/mac/libcustomssl.a',
							],
						},
					],
					[
						'OS == "ios"',
						{
							'libraries':
							[
								'lib/ios/$(SDK_NAME)/libcustomcrypto.a',
								'lib/ios/$(SDK_NAME)/libcustomssl.a',
							],
						},
					],
					[
						'OS == "linux"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'library_dirs':
							[
								'lib/linux/<(target_arch)',
							],
							
							'libraries':
							[
								'-Wl,-whole-archive',
								'-lcustomcrypto',
								'-lcustomssl',
								'-Wl,-no-whole-archive',
							],
						},
					],
					[
						'OS == "android"',
						{
							'library_dirs':
							[
								'lib/android/<(target_arch)/api<(android_api_version)',
							],
							
							'libraries':
							[
								'-Wl,-whole-archive',
								'-lcustomcrypto',
								'-lcustomssl',
								'-Wl,-no-whole-archive',
							],
						},
					],
					[
						'OS == "win"',
						{
							'library_dirs':
							[
								'unpacked/openssl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
							],
							
							'libraries':
							[
								'-llibeay32',
								'-lssleay32',
							],
						},
					],
				],
			},
		},
	],
}
