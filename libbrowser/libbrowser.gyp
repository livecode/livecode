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
				'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
				'../thirdparty/libcef/libcef.gyp:libcef_stubs',
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
				'src/libbrowser_cef_osx.mm',
				'src/libbrowser_cef_win.cpp',
				'src/libbrowser_cefshared_osx.cpp',
				'src/libbrowser_cefshared_lnx.cpp',
				
				'src/libbrowser_win.rc.h',
				'src/libbrowser_win.rc',
				
				'src/signal_restore_posix.cpp',
				'src/WebAuthenticationPanel.m',

				'src/libbrowser_uiwebview.h',
				'src/libbrowser_uiwebview.mm',
				
				'src/libbrowser_android.cpp',
				
				'src/libbrowser_desktop_factories.cpp',
				'src/libbrowser_ios_factories.cpp',
			],
			
			'conditions':
			[
				## Exclusions
				# Only use CEF on desktop platforms
				[
					'OS != "mac" and OS != "win" and OS != "linux"',
					{
						'sources!':
						[
							'src/libbrowser_cef.cpp',
							'src/libbrowser_desktop_factories.cpp',
						],
					},
				],
				
				[
					'OS != "mac"',
					{
						'sources!':
						[
							'src/libbrowser_cef_osx.mm',
							'src/libbrowser_cefshared_osx.cpp',
							'src/WebAuthenticationPanel.m',
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
						'dependencies':
						[
							'libbrowser-cefprocess-helpers',
						],
					
						# Copy the CEF processes and framework into the expected place
						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)/Frameworks',
								'files':
								[
									'<(PRODUCT_DIR)/libbrowser-cefprocess.app',
									'<(PRODUCT_DIR)/libbrowser-cefprocess EH.app',
									'<(PRODUCT_DIR)/libbrowser-cefprocess NP.app',
									'$(SOLUTION_DIR)/prebuilt/lib/mac/Chromium Embedded Framework.framework',
								],
							},
						],
					
						'all_dependent_settings':
						{
							'variables':
							{
								###'dist_files': [ '<(PRODUCT_DIR)/<(product_name).bundle' ],
								'dist_aux_files': [ '<(PRODUCT_DIR)/Frameworks' ],
							},
						},
					},
				],
				
				[
					'OS == "mac" or OS == "win" or OS == "linux"',
					{
						'dependencies':
						[
							'libbrowser-cefprocess',
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
			
			'mac_bundle_resources':
			[
				'/src/com_livecode_libbrowser_WebAuthenticationPanel.nib',
			],
			
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
				'src/libbrowser_cefprocess_osx.mm',
				'src/libbrowser_cefprocess_win.cpp',
				'src/libbrowser_cefshared_lnx.cpp',
				'src/libbrowser_cefshared_osx.cpp',
			],
			
			'conditions':
			[
				# OSX, Windows, and Linux only
				[
					'OS != "mac" and OS != "win" and OS != "linux"',
					{
						'type': 'none',
						'mac_bundle': 0,
					},
				],
				
				## Exclusions
				[
					'OS != "mac"',
					{
						'sources!':
						[
							'src/libbrowser_cefprocess_osx.mm',
							'src/libbrowser_cefshared_osx.cpp',
						],
					},
				],
				
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
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/libbrowser-cefprocess-Info.plist',
			},
		}
	],

	'conditions':
	[
		# CEF on OSX needs some helper applications
		[
			'OS == "mac"',
			{
				'targets':
				[
					{
						'target_name': 'libbrowser-cefprocess-helpers',
						'type': 'none',
						
						'dependencies':
						[
							'libbrowser-cefprocess',
						],
						
						'actions':
						[
							# Create the EH and NP variants of the CEF process
							{
								'action_name': 'create_cefprocess_variants',
								'inputs':
								[
									'<(PRODUCT_DIR)/libbrowser-cefprocess.app',
									'tools/make_more_helpers.sh',
								],
								'outputs':
								[
									'<(PRODUCT_DIR)/libbrowser-cefprocess EH.app',
									'<(PRODUCT_DIR)/libbrowser-cefprocess NP.app',
								],
								
								'action':
								[
									'tools/make_more_helpers.sh',
									'<(PRODUCT_DIR)',
									'libbrowser-cefprocess',
								],
							},
						],
					},
				],
			},
		],
	],
}
