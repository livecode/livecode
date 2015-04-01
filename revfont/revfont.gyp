{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revfont',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'sources':
			[
				'src/revfont.h',
				'src/revfont.cpp',
				'src/osxfont.cpp',
				'src/w32font.cpp',
			],
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
						],
					},
				],
				[
					'OS == "win"',
					{
						'libraries':
						[
							'-lgdi32',
							'-luser32',
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revfont-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revfont.exports',
			},
		},
	],
}
