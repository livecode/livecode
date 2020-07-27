{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'external-revpdfprinter',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revpdfprinter',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_cairo',
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
				'src/revpdfprinter_android.cpp',
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
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-lfontconfig',
							'-lfreetype',
							'-lgobject-2.0',
							'-lpangoft2-1.0',
						],
					},
				],
				[
					'OS == "android"',
					{
						'product_name': 'RevPdfPrinter',
						'product_extension': '',

						'dependencies':
						[
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_freetype',
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
