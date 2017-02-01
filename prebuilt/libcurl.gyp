{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libcurl',
			'type': 'none',
			
			'dependencies':
			[
				'fetch.gyp:fetch',
				'libopenssl.gyp:libopenssl',
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
								'unpacked/curl/<(target_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
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
								'$(SDKROOT)/usr/lib/libcurl.dylib',
							],
						},
					],
					[
						'OS == "linux"',
						{
							'library_dirs':
							[
								'lib/linux/<(target_arch)',
							],
							
							'libraries':
							[
								'-lcurl',
								'-lrt',
							],
						},
					],
					[
						'OS == "win"',
						{
							'library_dirs':
							[
								'unpacked/curl/<(target_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
							],
							
							'libraries':
							[
								'-llibcurl_a',
							],
						},
					],
				],
			},
		},
	],
}
