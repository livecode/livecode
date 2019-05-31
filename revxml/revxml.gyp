{
	'includes':
	[
		'../common.gypi',
	],
	
	'variables':
	{
		'revxml_sources':
		[
			'src/cxml.h',
			'src/revxml.h',
			'src/revxml.cpp',
			'src/xmlattribute.cpp',
			'src/xmldoc.cpp',
			'src/xmlelement.cpp',
		],
	},
	
	'targets':
	[
		{
			'target_name': 'external-revxml',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revxml',
			
			'dependencies':
            [
                '../libexternal/libexternal.gyp:libExternal',
                '../libexternal/libexternal.gyp:libExternal-symbol-exports',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_xml',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_xslt',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_z',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(revxml_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revxml-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revxml.exports',
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		
			'conditions':
			[
				[
					'OS == "android"',
					{
						'product_name': 'RevXml',
						'product_extension': '',
					},
				],
			],
		},
		{
			'target_name': 'external-revxml-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-revxml',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
                '../libexternal/libexternal.gyp:libExternal-symbol-exports',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_xml',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_xslt',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_z',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(revxml_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revxml-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revxml.exports',
			},

			'conditions':
			[
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
	],
}
