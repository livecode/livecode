{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'external-revbrowser',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revbrowser',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/cefbrowser.h',
				'src/cefbrowser_msg.h',
				'src/cefshared.h',
				'src/osxbrowser.h',
				'src/revbrowser.h',
				'src/revbrowser.rc.h',
				'src/signal_restore_posix.h',
				'src/w32browser.h',
				
				'src/cefbrowser.cpp',
				'src/cefbrowser_lnx.cpp',
				'src/cefbrowser_w32.cpp',
				'src/cefshared_lnx.cpp',
				'src/cefshared_w32.cpp',
				'src/lnxbrowser.cpp',
				'src/osxbrowser.mm',
				'src/revbrowser.cpp',
				'src/signal_restore_posix.cpp',
				'src/w32browser.cpp',
				'src/revbrowser.rc',
			],
			
			'conditions':
			[
				# Only supported on OSX, Windows and Linux
				[
					'OS != "mac" and OS != "win" and OS != "linux"',
					{
						'type': 'none',
					},
				],
				# CEF only supported on Windows & Linux
				[
					'OS == "win" or OS == "linux"',
					{
						'dependencies':
						[
							'../prebuilt/libcef.gyp:libcef',
							'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
							'../thirdparty/libcef/libcef.gyp:libcef_stubs',

							'revbrowser-cefprocess',
						],
					},
					# else
					{
						'sources!':
						[
							'src/cefbrowser.h',
							'src/cefbrowser_msg.h',
							'src/cefshared.h',

							'src/cefbrowser.cpp',
							'src/cefbrowser_lnx.cpp',
							'src/cefbrowser_w32.cpp',
							'src/cefshared_lnx.cpp',
							'src/cefshared_w32.cpp',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
						],
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).bundle' ],
							},
						},
					},
				],
				[
					'OS == "win"',
					{
						'defines':
						[
							'__EXCEPTIONS',
						],
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).dll' ],
							},
						},
					},
				],
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-ldl',
							'-lX11',
						],
		
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).so' ],
							},
						},
					},
				],
			],
			
			'cflags_cc!':
			[
				'-fno-rtti',
				'-fno-exceptions',
			],
			
			'msvs_settings':
			{
				'VCCLCompilerTool':
				{
					'ExceptionHandling': '1',	# /EHsc
				},	
			},
			
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revbrowser-Info.plist',
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
			},
		},
		
		{
			'target_name': 'revbrowser-cefprocess',
			'type': 'executable',
			'product_name': 'revbrowser-cefprocess',
			
			# Windows and Linux only
			'conditions':
			[
				[
					'OS != "win" and OS != "linux"',
					{
						'type': 'none',
					},
				],
				[
					'OS == "win" or OS == "linux"',
					{
						# Distributing the OSX version is done separately
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)' ],
							},
						},
					},
				],
			],
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
				'../thirdparty/libcef/libcef.gyp:libcef_stubs',
			],
			
			'sources':
			[
				'src/cefprocess.cpp',
				'src/cefprocess_lnx.cpp',
				'src/cefprocess_w32.cpp',
				'src/cefshared_lnx.cpp',
				'src/cefshared_w32.cpp',
			],
		},
	],
}
