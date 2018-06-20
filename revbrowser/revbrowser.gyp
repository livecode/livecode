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
				'src/osxbrowser.h',
				'src/revbrowser.h',
				'src/revbrowser.rc.h',
				'src/signal_restore_posix.h',
				'src/w32browser.h',
				
				'src/cefbrowser.cpp',
				'src/cefbrowser_lnx.cpp',
				'src/cefbrowser_w32.cpp',
				'src/lnxbrowser.cpp',
				'src/osxbrowser.mm',
				'src/revbrowser.cpp',
				'src/signal_restore_posix.cpp',
				'src/w32browser.cpp',
				'src/revbrowser.rc',
			],
			
			'target_conditions':
			[
				# Only supported on OSX, Windows and Linux
				[
					'not toolset_os in ("mac", "win", "linux")',
					{
						'type': 'none',
					},
				],
				# CEF only supported on Windows & Linux
				[
					'not toolset_os in ("win", "linux") or (toolset_os == "linux" and not toolset_arch in ("x86", "x86_64"))',
					{
						'sources!':
						[
							'src/cefbrowser.h',
							'src/cefbrowser_msg.h',

							'src/cefbrowser.cpp',
							'src/cefbrowser_lnx.cpp',
							'src/cefbrowser_w32.cpp',
						],
					},
				],
				[
					'toolset_os == "mac"',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
						],
					},
				],
				[
					'toolset_os == "win"',
					{
						'copies':
						[
							{
								'destination':'<(PRODUCT_DIR)/Externals/CEF/',
								'files':
								[
									'<(PRODUCT_DIR)/revbrowser-cefprocess.exe',
								],
							},
						],
						
						'defines':
						[
							'__EXCEPTIONS',
						],
					},
				],
				[
					'toolset_os == "linux" and toolset_arch in ("x86", "x86_64")',
					{
                        'copies':
                        [
                            {
                                'destination':'<(PRODUCT_DIR)/',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/revbrowser-cefprocess',
                                ],
                            },
                        ],
                        
						'libraries':
						[
							'-ldl',
							'-lX11',
						],
					},
				],
			],
			
			'conditions':
			[
				[
					# Only the CEF platforms need revbrowser-cefprocess
					'OS in ("linux", "win") or host_os in ("linux", "win")',
					{
						'dependencies':
						[
							'../prebuilt/libcef.gyp:libcef',
							'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
							'../thirdparty/libcef/libcef.gyp:libcef_stubs',

							'revbrowser-cefprocess',
						],
					},
				],
			],
						
			'all_dependent_settings':
			{
				'conditions':
				[
					[
						'OS == "win"',
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).dll' ],
							},
						},
					],
					[
						'OS == "linux" and target_arch in ("x86", "x86_64")',
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).so' ],
							},
						},
					],
					[
						'OS == "mac"',
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).bundle' ],
							},
						},
					],
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
					'ExceptionHandling': '1',	# /EHsc
				},	
			},
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revbrowser-Info.plist',
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
			},
		},
	],
		
    'conditions':
    [
        [
            'OS in ("linux", "win") or host_os in ("linux", "win")',
            {
                'targets':
                [
					{
						'target_name': 'revbrowser-cefprocess',
						'type': 'executable',
						'product_name': 'revbrowser-cefprocess',
			
						# Windows and Linux only
						'target_conditions':
						[
							[
								'not toolset_os in ("win", "linux") or (toolset_os == "linux" and not toolset_arch in ("x86", "x86_64"))',
								{
									'type': 'none',
								},
							],
				
							[
								'toolset_os == "win"',
								{	
									'library_dirs':
									[
										'../prebuilt/unpacked/cef/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib/CEF/',
									],

									'libraries':
									[
										'-llibcef.lib',
									],
								},
							],
				
							[
								'toolset_os == "linux"',
								{
									'library_dirs':
									[
										'../prebuilt/lib/linux/<(target_arch)/CEF/',
									],
						
									'libraries':
									[
										'-lcef',
									],
					   
									'ldflags':
									[
										'-Wl,--allow-shlib-undefined',
										'-Wl,-rpath=\\$$ORIGIN',
									],
								},
							],
				
						],
			
						'all_dependent_settings':
						{
							'conditions':
							[
								[
									'OS == "win" or (OS == "linux" and target_arch in ("x86", "x86_64"))',
									{
										# Distributing the OSX version is done separately
										'variables':
										{
											'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)' ],
										},
									}
								],
							],
						},
			
						'dependencies':
						[
							'../libcore/libcore.gyp:libCore',
							'../libfoundation/libfoundation.gyp:libFoundation',
							'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
							'../prebuilt/libcef.gyp:libcef',
							'../prebuilt/libicu.gyp:libicu',
						],
			
						'sources':
						[
							'src/cefprocess.cpp',
							'src/cefprocess_lnx.cpp',
							'src/cefprocess_w32.cpp',
						],
					},
                ],
            },
        ],
	],
}
