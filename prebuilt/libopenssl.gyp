{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libopenssl',
			'type': 'none',

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libopenssl/include',
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
								'<(src_top_dir_abs)/prebuilt/lib/linux/<(target_arch)',
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
								'<(src_top_dir_abs)/prebuilt/lib/android/<(target_arch)',
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
								'lib/win32/<(target_arch)',
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
