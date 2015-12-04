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
				'../prebuilt/libcef.gyp:libcef',
				'../thirdparty/libcef/libcef.gyp:libcef_library_wrapper',
				'../thirdparty/libcef/libcef.gyp:libcef_stubs',
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
				'src/WebAuthenticationPanel.h',
				
				'src/cefbrowser.cpp',
				'src/cefbrowser_lnx.cpp',
				'src/cefbrowser_osx.mm',
				'src/cefbrowser_w32.cpp',
				'src/cefshared_lnx.cpp',
				'src/cefshared_osx.cpp',
				'src/cefshared_w32.cpp',
				'src/lnxbrowser.cpp',
				'src/osxbrowser.mm',
				'src/revbrowser.cpp',
				'src/signal_restore_posix.cpp',
				'src/w32browser.cpp',
				'src/revbrowser.rc',
				'src/WebAuthenticationPanel.m',
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
				[
					'OS == "mac"',
					{
						'dependencies':
						[
							'revbrowser-cefprocess-helpers',
						],
						
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/WebKit.framework',
						],
						
						'xcode_settings':
						{
							# Latest SDK that this can compile against is 10.9 but
							# we currently use 10.8 and keep that here to avoid
							# having to do yet more SDK downloading.
							'SDKROOT': 'macosx10.8',
						},

						# Copy the CEF processes and framework into the expected place
						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)/Frameworks',
								'files':
								[
									'<(PRODUCT_DIR)/revbrowser-cefprocess.app',
									'<(PRODUCT_DIR)/revbrowser-cefprocess EH.app',
									'<(PRODUCT_DIR)/revbrowser-cefprocess NP.app',
									'$(SOLUTION_DIR)/prebuilt/lib/mac/Chromium Embedded Framework.framework',
								],
							},
						],
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name).bundle' ],
								'dist_aux_files': [ '<(PRODUCT_DIR)/Frameworks' ],
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
			
			'mac_bundle_resources':
			[
				'src/com_runrev_livecode_WebAuthenticationPanel.nib',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revbrowser-Info.plist',
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
			},
		},
		
		{
			'target_name': 'revbrowser-cefprocess',
			'type': 'executable',
			'mac_bundle': 1,
			'product_name': 'revbrowser-cefprocess',
			
			# OSX, Windows and Linux only
			'conditions':
			[
				[
					'OS != "mac" and OS != "win" and OS != "linux"',
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
				'src/cefprocess_osx.mm',
				'src/cefprocess_w32.cpp',
				'src/cefshared_lnx.cpp',
				'src/cefshared_osx.cpp',
				'src/cefshared_w32.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'revbrowser-cefprocess-Info.plist',
			},
		},
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
						'target_name': 'revbrowser-cefprocess-helpers',
						'type': 'none',
						
						'dependencies':
						[
							'revbrowser-cefprocess',
						],
						
						'actions':
						[
							# Create the EH and NP variants of the CEF process
							{
								'action_name': 'create_cefprocess_variants',
								'inputs':
								[
									'<(PRODUCT_DIR)/revbrowser-cefprocess.app',
									'tools/make_more_helpers.sh',
								],
								'outputs':
								[
									'<(PRODUCT_DIR)/revbrowser-cefprocess EH.app',
									'<(PRODUCT_DIR)/revbrowser-cefprocess NP.app',
								],
								
								'action':
								[
									'tools/make_more_helpers.sh',
									'<(PRODUCT_DIR)',
									'revbrowser-cefprocess',
								],
							},
						],
					},
				],
			},
		],
	],
}
