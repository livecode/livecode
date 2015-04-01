{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revpdfprinter',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libcairo/libcairo.gyp:libcairo',
			],
			
			'include_dirs':
			[
				'src',
				'../engine/include',
			],
			
			'sources':
			[
				'src/revpdfprinter.h',
				'src/revpdfprinter.cpp',
				'src/revpdfprinter_lnx.cpp',
				'src/revpdfprinter_osx.cpp',
				'src/revpdfprinter_w32.cpp',
				'src/revpdfprinter_coretext.mm',
				'src/revpdfprinter_ios.mm',
			],
			
			# Even on OSX, we don't use the non-coretext method any more
			'sources!':
			[
				'src/revpdfprinter_osx.cpp',
				'src/revpdfprinter_ios.mm',
			],
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/CoreText.framework',
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revpdfprinter-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revpdfprinter.exports',
			},
			
			'variables':
			{
				'ios_external_symbols': [ '_MCCustomPrinterCreate' ],
			},
		},
	],
}
