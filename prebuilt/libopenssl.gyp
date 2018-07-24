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

			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "win"',
						{
							'include_dirs':
							[
								'unpacked/openssl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'toolset_os != "win"',
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

			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "win"',
						{
							'include_dirs':
							[
								'unpacked/openssl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'toolset_os != "win"',
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
				'target_conditions':
				[
					[
						'toolset_os == "mac"',
						{
							'libraries':
							[
								'lib/mac/libcustomcrypto.a',
								'lib/mac/libcustomssl.a',
							],
						},
					],
					[
						'toolset_os == "ios"',
						{
							'libraries':
							[
								'lib/ios/$(SDK_NAME)/libcustomcrypto.a',
								'lib/ios/$(SDK_NAME)/libcustomssl.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'library_dirs':
							[
								'lib/linux/>(toolset_arch)',
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
						'toolset_os == "android"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'conditions':
							[
								[
									'OS == "android"',
									{
										'library_dirs':
										[
											'lib/android/<(target_arch)/<(android_subplatform)',
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
							],
						},
					],
					[
						'toolset_os == "win"',
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
