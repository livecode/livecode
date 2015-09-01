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
				'include_dirs':
				[
					'../thirdparty/libcurl/include',
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
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'library_dirs':
							[
								'<(src_top_dir_abs)/prebuilt/lib/linux/<(target_arch)',
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
								'lib/win32/<(target_arch)',
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
