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

			'toolsets': ['host', 'target'],
			
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
				
				'src/libbrowser_win.rc.h',
				'src/libbrowser_win.rc',
				
				'src/signal_restore_posix.cpp',

				'src/libbrowser_osx_webview.h',
				'src/libbrowser_osx_webview.mm',
				
				'src/libbrowser_wkwebview.h',
				'src/libbrowser_wkwebview.mm',

				'src/libbrowser_nsvalue.h',
				'src/libbrowser_nsvalue.mm',
				
				'src/libbrowser_android.cpp',
				
				'src/libbrowser_lnx_factories.cpp',
				'src/libbrowser_win_factories.cpp',
				'src/libbrowser_osx_factories.cpp',
				'src/libbrowser_ios_factories.cpp',
			],
			
			'target_conditions':
			[
				## Exclusions
				# Only use CEF on desktop platforms
				[
					'not (toolset_os == "win" or (toolset_os == "linux" and toolset_arch in ("x86", "x86_64")))',
					{
						'sources!':
						[
							'src/libbrowser_cef.cpp',
						],
					},
				],
				
				[
					'toolset_os != "mac"',
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
					'not toolset_os in ["mac", "ios"]',
					{
						'sources!':
						[
							'src/libbrowser_nsvalue.h',
							'src/libbrowser_nsvalue.mm',
						],
					},
				],
				
				[
					'toolset_os != "win"',
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
					'toolset_os != "linux"',
					{
						'sources!':
						[
							'src/libbrowser_cef_lnx.cpp',
							'src/signal_restore_posix.cpp',
							
							'src/libbrowser_lnx_factories.cpp',
						],
					},
				],

				[
					'toolset_os == "linux" and not toolset_arch in ("x86", "x86_64")',
					{
						'sources!':
						[
							'src/libbrowser_cef_lnx.cpp',
						],
					},
				],
				
				[
					'toolset_os != "ios"',
					{
						'sources!':
						[
							'src/libbrowser_wkwebview.h',
							'src/libbrowser_wkwebview.mm',

							'src/libbrowser_ios_factories.cpp',
						],
					},
				],
				
				[
					'toolset_os != "android"',
					{
						'sources!':
						[
							'src/libbrowser_android.cpp',
						],
					},
				],
			],

			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "mac"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/JavaScriptCore.framework',
							],
						},
					],
					[
						'toolset_os == "ios"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
							],
						},
					],
				],
			},
			
			# Gyp doesn't like dependencies in 'target_conditions'...
			'conditions':
			[
				[
					# Only the CEF platforms need libbrowser-cefprocess
					'OS in ("linux", "win") or host_os in ("linux", "win")',
					{
						'dependencies':
						[
							'libbrowser-cefprocess',
							'../prebuilt/libcef.gyp:libcef',
							'../prebuilt/libicu.gyp:libicu',
							'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
							'../thirdparty/libcef/libcef.gyp:libcef_stubs',
						],
					},
				],
				
				[
					'OS == "win"',
					{	
						'copies':
						[
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
    ],
    
    'conditions':
    [
        [
            'OS in ("linux", "win") or host_os in ("linux", "win")',
            {
                'targets':
                [
                    {
                        'target_name': 'libbrowser-cefprocess',
                        'type': 'executable',
                        'mac_bundle': 1,
                        'product_name': 'libbrowser-cefprocess',
                        
			'toolsets': ['host', 'target'],

                        'dependencies':
                        [
                            '../libcore/libcore.gyp:libCore',
                            '../libfoundation/libfoundation.gyp:libFoundation',
                            '../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
                            '../prebuilt/libcef.gyp:libcef',
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
                        ],
                        
                        'target_conditions':
                        [
                            [
								'toolset_os not in ("win", "linux")',
								{
									'type': 'none',
								},
							],
							## Exclusions
                            [
                                'toolset_os != "win"',
                                {
                                    'sources!':
                                    [
                                        'src/libbrowser_cefprocess_win.cpp',
                                    ],
                                },
                            ],
                            
                            [
                                'toolset_os != "linux"',
                                {
                                    'sources!':
                                    [
                                        'src/libbrowser_cefprocess_lnx.cpp',
                                    ],
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
                                        '../prebuilt/lib/linux/>(toolset_arch)/CEF/',
                                    ],
                    
                                    'libraries':
                                    [
                                        '-lcef',
                                    ],
                                   
                                    'ldflags':
                                    [
                                        '-Wl,--allow-shlib-undefined',
                                        '-Wl,-rpath=\\$$ORIGIN/Externals/CEF',
                                    ],
                                },
                            ],

                            [
                                'toolset_os == "linux" and not toolset_arch in ("x86", "x86_64")',
                                {
                                    'type': 'none',
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
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)' ],
							},
						},
					],
				],
			},
                    },
                ],
            },
        ],
    ],
}
