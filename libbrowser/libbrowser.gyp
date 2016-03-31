{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libbrowser',
			'type': 'static_library',
			
			'dependencies':
			[
				# '../libcore/libcore.gyp:libCore',
			],
			
			'include_dirs':
			[
				'include',
				'../libcore/include',
			],
			
			'sources':
			[
				'include/libbrowser.h',
				
				'src/libbrowser.cpp',
				'src/libbrowser_internal.h',
				'src/libbrowser_memory.cpp',
				'src/libbrowser_value.cpp',

				'src/libbrowser_cef.cpp',
				'src/libbrowser_cef.h',
				'src/libbrowser_cef_lnx.cpp',
				'src/libbrowser_cef_win.cpp',
				'src/libbrowser_cefshared_lnx.cpp',
				
				'src/libbrowser_win.rc.h',
				'src/libbrowser_win.rc',
				
				'src/signal_restore_posix.cpp',

				'src/libbrowser_uiwebview.h',
				'src/libbrowser_uiwebview.mm',
				
				'src/libbrowser_osx_webview.h',
				'src/libbrowser_osx_webview.mm',
				
				'src/libbrowser_android.cpp',
				
				'src/libbrowser_lnx_factories.cpp',
				'src/libbrowser_win_factories.cpp',
				'src/libbrowser_osx_factories.cpp',
				'src/libbrowser_ios_factories.cpp',
			],
			
			'conditions':
			[
				## Exclusions
				# Only use CEF on desktop platforms
				[
					'OS != "win" and OS != "linux"',
					{
						'sources!':
						[
							'src/libbrowser_cef.cpp',
						],
					},
				],
				
				[
					'OS != "mac"',
					{
						'sources!':
						[

							'src/libbrowser_osx_webview.h',
							'src/libbrowser_osx_webview.mm',
							
							'src/libbrowser_osx_factories.cpp',
						],
					},
				],
				
				[
					'OS != "win"',
					{
						'sources!':
						[
							'src/libbrowser_cef_win.cpp',
							'src/libbrowser_win.rc.h',
							'src/libbrowser_win.rc',
							
							'src/libbrowser_win_factories.cpp',
						],
					},
				],
				
				[
					'OS != "linux"',
					{
						'sources!':
						[
							'src/libbrowser_cef_lnx.cpp',
							'src/libbrowser_cefshared_lnx.cpp',
							'src/signal_restore_posix.cpp',
							
							'src/libbrowser_lnx_factories.cpp',
						],
					},
				],
				
				[
					'OS != "ios"',
					{
						'sources!':
						[
							'src/libbrowser_uiwebview.mm',
							'src/libbrowser_ios_factories.cpp',
						],
					},
				],
				
				[
					'OS != "android"',
					{
						'sources!':
						[
							'src/libbrowser_android.cpp',
						],
					},
				],
				
				[
					'OS == "mac"',
					{
						'link_settings':
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/JavaScriptCore.framework',
							],
						},
					},
				],
				
				[
					# Only the CEF platforms need libbrowser-cefprocess
					'OS == "win" or OS == "linux"',
					{
						'dependencies':
						[
							'libbrowser-cefprocess',
							'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
							'../thirdparty/libcef/libcef.gyp:libcef_stubs',
						],
					},
				],
				
				# Copy files needed to run from build folder
				[
					'OS == "linux"',
					{
						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)',
								'files': [
									'../prebuilt/lib/linux/<(target_arch)/CEF/icudtl.dat',
									'../prebuilt/lib/linux/<(target_arch)/CEF/natives_blob.bin',
									'../prebuilt/lib/linux/<(target_arch)/CEF/snapshot_blob.bin',
								],
							},
							{
								'destination': '<(PRODUCT_DIR)/Externals/',
								'files': [
									'../prebuilt/lib/linux/<(target_arch)/CEF',
								],
							}
						],
					},
				],
				
				[
					'OS == "win"',
					{
						'copies':
						[
							{
								'destination':'<(PRODUCT_DIR)/Externals/',
								'files':
								[
									'../prebuilt/lib/win32/<(target_arch)/CEF/',
								],
							},
							{
								'destination':'<(PRODUCT_DIR)/Externals/CEF/',
								'files':
								[
									'<(PRODUCT_DIR)/libbrowser-cefprocess.exe',
								],
							},
						],
					},
				],
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
			
			'cflags_cc!':
			[
				'-fno-rtti',
				'-fno-exceptions',
			],
			
			'msvs_settings':
			{
				'VCCLCompilerTool':
				{
					'ExceptionHandling': '1',  # /EHsc
				},
			},
			
			
			'xcode_settings':
			{
#				'INFOPLIST_FILE': 'rsrc/libbrowser-Info.plist',
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
			},
		},
		
		{
			'target_name': 'libbrowser-cefprocess',
			'type': 'executable',
			'mac_bundle': 1,
			'product_name': 'libbrowser-cefprocess',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
				'../thirdparty/libcef/libcef.gyp:libcef_stubs',
			],

			'include_dirs':
			[
				'include',
			],
			
			'sources':
			[
				'src/libbrowser_memory.cpp',
				'src/libbrowser_cefprocess.cpp',
				'src/libbrowser_cefprocess_lnx.cpp',
				'src/libbrowser_cefprocess_win.cpp',
				'src/libbrowser_cefshared_lnx.cpp',
			],
			
			'conditions':
			[
				## Exclusions
				[
					'OS != "win"',
					{
						'sources!':
						[
							'src/libbrowser_cefprocess_win.cpp',
						],
					},
				],
				
				[
					'OS != "linux"',
					{
						'sources!':
						[
							'src/libbrowser_cefprocess_lnx.cpp',
							'src/libbrowser_cefshared_lnx.cpp',
						],
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
		},
	],
}
