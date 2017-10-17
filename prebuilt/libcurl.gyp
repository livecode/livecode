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
			
			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
				'libopenssl.gyp:libopenssl',
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
								'unpacked/curl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'toolset_os != "win"',
						{
							'include_dirs':
							[
								'../thirdparty/libcurl/include',
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
								'$(SDKROOT)/usr/lib/libcurl.dylib',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'library_dirs':
							[
								'lib/linux/>(toolset_arch)',
							],
							
							'libraries':
							[
								'-lcurl',
								'-lrt',
							],
						},
					],
					[
						'toolset_os == "win"',
						{
							'library_dirs':
							[
								'unpacked/curl/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
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
