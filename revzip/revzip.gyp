{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revzip',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revzip',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libzip/libzip.gyp:libzip',
			],
			
			'sources':
			[
				'src/revzip.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revzip-info.plist',
			},
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'product_extension': 'bundle',
						
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
						],
					},
				],
			],
			
			'variables':
			{
				'ios_external_symbols': [ '_getXtable' ],
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		},
	],
}
