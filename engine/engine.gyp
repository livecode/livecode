{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
	],
	
	'target_defaults':
	{
		'conditions':
		[
			[
				'OS == "linux" or OS == "android" or OS == "emscripten"',
				{
					# Ensure that the symbols LCB binds to are exported from the engine
					'ldflags': [ '-rdynamic' ],
				},
			],
		],
	},
	
	'targets':
	[
		{
			'target_name': 'host-server',
			'type': 'none',

			'toolsets': ['host', 'target'],

			'conditions':
			[
				[
					'cross_compile != 0',
					{
						'dependencies':
						[
							'server#host',
						],

						'direct_dependent_settings':
						{
							'variables':
							{
								'engine': '<(PRODUCT_DIR)/server-community-host>(exe_suffix)'
							},
						},
					},
					{
						'dependencies':
						[
							'server#target',
						],

						'direct_dependent_settings':
						{
							'variables':
							{
								'engine': '<(PRODUCT_DIR)/server-community>(exe_suffix)',
							},
						},
					},
				],
			],
		},

		
		{
			'target_name': 'descriptify_environment_stack',
			'type': 'none',
			
			'sources':
			[
				'src/environment.livecode',
				'../ide-support/revliburl.livecodescript',
				'src/environment/accountsignupcardbehavior.livecodescript',
				'src/environment/automaticactivationactivationgroupbehavior.livecodescript',
				'src/environment/automaticactivationactivationinputpassfieldbehavior.livecodescript',
				'src/environment/automaticactivationbackgroundgroupbehavior.livecodescript',
				'src/environment/automaticactivationcardbehavior.livecodescript',
				'src/environment/automaticactivationprocessinggroupbehavior.livecodescript',
				'src/environment/getstartedgetstartedstatusgroupbehavior.livecodescript',
				'src/environment/resourcesresourcesloadergroupbehavior.livecodescript',
				'src/environment/resourcesresourcespushbuttonbehaviorbuttonbehavior.livecodescript',
				'src/environment/stackbehavior.livecodescript',
				'src/environment/unlicensedactivatefieldbehavior.livecodescript',
				'src/environment/unlicensedbkgndbehavior.livecodescript',
				'src/environment/unlicensedbuynowbuttonbehavior.livecodescript',
			],
			
			'dependencies':
			[
				# Requires a working LiveCode engine
				'host-server',
			],
			
			'actions':
			[
				{
					'action_name': 'descriptify_environment_stack',
					'message': 'De-scriptifying the environment stack',
					
					'inputs':
					[
						'../util/descriptify_stack.livecodescript',
						'<@(_sources)',
					],
					
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/environment_descriptified.livecode',
					],
					
					'action':
					[
						'>(engine)',
						'../util/descriptify_stack.livecodescript',
						'<(SHARED_INTERMEDIATE_DIR)/src/environment_descriptified.livecode',
						'<@(_sources)',
						
					],
				},
			],
		},
		
		{
			'target_name': 'encode_environment_stack',
			'type': 'none',
			
			'dependencies':
			[
				'descriptify_environment_stack',
			],
			
			'actions':
			[
				{
					'action_name': 'encode_environment_stack',
					'inputs':
					[
						'../util/compress_data.pl',
						'<(SHARED_INTERMEDIATE_DIR)/src/environment_descriptified.livecode',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/compress_data.pl',
						'<(SHARED_INTERMEDIATE_DIR)/src/environment_descriptified.livecode',
						'<@(_outputs)',
						# Really nasty hack to prevent this from being treated as a path
						'$(this_is_an_undefined_variable)MCstartupstack',
					],
				},
			],
		},
				
		{
			'target_name': 'server',
			'type': 'executable',
			'product_name': 'server-community',
			
			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'kernel-server.gyp:kernel-server',
				
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../libgraphics/libgraphics.gyp:libGraphics',

				'lcb-modules.gyp:engine_lcb_modules',
			],
			
			'sources':
			[
				'<@(engine_security_source_files)',
				'>@(builtin_lcb_modules)',
				'src/main.cpp',
			],

			'include_dirs':
			[
				'../libfoundation/include',
			],

			'conditions':
			[
				[
					'mobile != 0',
					{
						'type': 'none',
						'mac_bundle': 0,
					},
				],
			],

			'target_conditions':
			[
				[
					'_toolset == "host"',
					{
						'product_name': 'server-community-host',
					},
				],
			],
			
			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)' ],
				},
			},
		},
		
		{
			'target_name': 'standalone',
			'product_name': 'standalone-community',

			'includes':
			[
				'app-bundle-template.gypi',
			],
			
			'variables':
			{
				'app_plist': 'rsrc/Standalone-Info.plist',
			},
			
			'dependencies':
			[
				'kernel-standalone.gyp:kernel-standalone',
				'engine-common.gyp:security-community',
				'lcb-modules.gyp:engine_lcb_modules',
			],
			
			'sources':
			[
				'>@(builtin_lcb_modules)',
				'src/dummy.cpp',
				'rsrc/standalone.rc',
			],

			'conditions':
			[
				[
					'OS != "win" and OS != "android"',
					{
						'sources':
						[
							'src/main.cpp',
						],
						'include_dirs':
						[
							'../libfoundation/include',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'product_name': 'Standalone-Community',
						'mac_bundle_resources':
						[
							'rsrc/Standalone.icns',
							'rsrc/StandaloneDoc.icns',
						],
					},
				],
				[
					'OS == "ios"',
					{
						'product_name': 'standalone-mobile-lib-community',
						'product_prefix': '',
						'product_extension': 'lcext',
						'app_plist': 'rsrc/standalone-mobile-Info.plist',
						
						# Forces all dependencies to be linked properly
						'type': 'shared_library',
						
						'variables':
						{
							'deps_file': '${SRCROOT}/standalone.ios',
						},

						'xcode_settings':
						{
							'DEAD_CODE_STRIPPING': 'NO',
							'DYLIB_COMPATIBILITY_VERSION': '',
							'DYLIB_CURRENT_VERSION': '',
							'MACH_O_TYPE': 'mh_object',
							'LINK_WITH_STANDARD_LIBRARIES': 'NO',
							'OTHER_LDFLAGS':
							[
								'-Wl,-sectcreate,__MISC,__deps,<(deps_file)',
								'-Wl,-u,_main',
								'-Wl,-u,_load_module',
								'-Wl,-u,_resolve_symbol',
								#'-all_load',		# Dead stripping later will remove un-needed symbols
							],
						},
					},
				],
				[
					# Use a linker script to add the project and payload sections to the Linux executable
					'OS == "linux"',
					{
						'ldflags':
						[
							'-Wl,-T,$(abs_srcdir)/engine/linux.link',
						],
					},
				],
				[
					# On Android, this needs to be built as a shared library
					'OS == "android"',
					{
						'product_name': 'Standalone-Community',
						'product_prefix': '',
						'product_extension': '',
						'product_dir': '<(PRODUCT_DIR)',	# Shared libraries are not placed in PRODUCT_DIR by default
						'type': 'loadable_module',		# Shared library imples --whole-archive
						
						'sources':
						[
							'engine/standalone-android.link',
						],
						
						'ldflags':
						[
							# Helpful for catching build problems
							'-Wl,-no-undefined',
							'-Wl,-T,$(abs_srcdir)/engine/standalone-android.link',
						],

						'actions':
						[
							{
								'action_name': 'copy_manifest',
								'message': 'Copying and update debuggable in manifest file',
								
								'inputs':
								[
									'rsrc/android-manifest.xml',
								],

								'outputs':
								[
									'<(PRODUCT_DIR)/Manifest.xml',
								],
								
								'action':
								[
									'../util/set_android_debuggable.sh',
									'<@(_inputs)',
									'<@(_outputs)',
								],
							},
							{
								'action_name': 'copy_inputcontrol',
								'message': 'Copying input control file',
								
								'inputs':
								[
									'rsrc/android-inputcontrol.xml',
								],
								
								'outputs':
								[
									'<(PRODUCT_DIR)/livecode_inputcontrol.xml',
								],
								
								'action':
								[
									'cp', '<@(_inputs)', '<@(_outputs)',
								],
							},
							{
								'action_name': 'copy_notify_icon',
								'message': 'Copying notification icon',
								
								'inputs':
								[
									'rsrc/android-notify-icon.png'
								],
								
								'outputs':
								[
									'<(PRODUCT_DIR)/notify_icon.png',
								],
								
								'action':
								[
									'cp', '<@(_inputs)', '<@(_outputs)',
								],
							},
							{
								'action_name': 'copy_nfc_tech_filter',
								'message': 'Copying NFC tech filter file',
								
								'inputs':
								[
									'rsrc/android-nfc_tech_filter.xml',
								],
								
								'outputs':
								[
									'<(PRODUCT_DIR)/nfc_tech_filter.xml',
								],
								
								'action':
								[
									'cp', '<@(_inputs)', '<@(_outputs)',
								],
							},
                            {
                                'action_name': 'copy_file_provider_paths',
                                'message': 'Copying file provider paths file',
                                
                                'inputs':
                                [
                                'rsrc/android-file_provider_paths.xml',
                                ],
                                
                                'outputs':
                                [
                                '<(PRODUCT_DIR)/file_provider_paths.xml',
                                ],
                                
                                'action':
                                [
                                'cp', '<@(_inputs)', '<@(_outputs)',
                                ],
                            },
						],
						
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files':
								[
									'<(PRODUCT_DIR)/Manifest.xml',
									'<(PRODUCT_DIR)/livecode_inputcontrol.xml',
									'<(PRODUCT_DIR)/notify_icon.png',
									'<(PRODUCT_DIR)/nfc_tech_filter.xml',
                                    '<(PRODUCT_DIR)/file_provider_paths.xml',
								],
							},
						},
					},
				],
				[
					'OS == "win"',
					{
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files':
								[
									'rsrc/w32-manifest-template.xml',
									'rsrc/w32-manifest-template-dpiaware.xml',
									'rsrc/w32-manifest-template-trustinfo.xml',
								],
							},
						},
					},
				],
				[
					'OS == "ios"',
					{
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files':
								[
									'rsrc/fontmap',
									'rsrc/mobile-device-template.plist',
									'rsrc/mobile-remote-notification-template.plist',
									'rsrc/mobile-template.plist',
									'rsrc/mobile-url-scheme-template.plist',
									'rsrc/mobile-disable-ats-template.plist',
									'rsrc/template-entitlements.xcent',
									'rsrc/template-beta-report-entitlement.xcent',
									'rsrc/template-remote-notification-entitlements.xcent',
									'rsrc/template-remote-notification-store-entitlements.xcent',
									'rsrc/template.storyboard',
								],
							},
						},
					},
				],
				[
					'OS == "emscripten"',
					{
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files':
								[
									'rsrc/emscripten-standalone-template/',
									'rsrc/emscripten-startup-template.livecodescript/',
									'<(PRODUCT_DIR)/standalone-community-<(version_string).js',
									'<(PRODUCT_DIR)/standalone-community-<(version_string).html',
									'<(PRODUCT_DIR)/standalone-community-<(version_string).html.mem',
								],
							},
						},

						'sources!':
						[
							'src/dummy.cpp',
						],
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'conditions':
					[
						[
							'OS == "android"',
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
							},
						],
						[
							'OS == "ios"',
							{
								'dist_files': [ '<(PRODUCT_DIR)/standalone-mobile-community.ios-engine' ],
							},
						],
						[
							'OS == "emscripten"',
							{
								'dist_files': [],
							}
						],
						[
							'OS != "android" and OS != "ios" and OS != "emscripten"',
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(app_bundle_suffix)' ],
							}
						],
					],
				},
			},
		},
		
		{
			'target_name': 'installer',
			'product_name': 'installer',
			
			'includes':
			[
				'app-bundle-template.gypi',
			],
			
			'variables':
			{
				'app_plist': 'rsrc/Installer-Info.plist',
			},
			
			'dependencies':
			[
				'kernel-installer.gyp:kernel-installer',
				'engine-common.gyp:security-community',
				'lcb-modules.gyp:engine_lcb_modules',
			],
			
			'sources':
			[
				'src/dummy.cpp',
				'rsrc/installer.rc',
				'>@(builtin_lcb_modules)',
			],

			'conditions':
			[
				[
					'OS != "win" and OS != "android"',
					{
						'sources':
						[
							'src/main.cpp',
						],
						'include_dirs':
						[
							'../libfoundation/include',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'product_name': 'Installer',
						'mac_bundle_resources':
						[
							'rsrc/Installer.icns',
						],
					},
				],
				[
					'mobile != 0',
					{
						'type': 'none',
						'mac_bundle': 0,
					},
				],
				[
					# Use a linker script to add the project and payload sections to the Linux executable
					'OS == "linux"',
					{
						'ldflags':
						[
							'-T', '$(abs_srcdir)/engine/linux.link',
						],
					},
				],
			],
			
			'msvs_settings':
			{
				'VCManifestTool':
				{
					'AdditionalManifestFiles': '$(ProjectDir)..\\..\\..\\engine\\src\\installer.manifest',
				},
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(app_bundle_suffix)' ],
				},
			},
		},

		{
			'target_name': 'development',
			'product_name': 'LiveCode-Community',

			'includes':
			[
				'app-bundle-template.gypi',
			],
			
			'variables':
			{
				'app_plist': 'rsrc/LiveCode-Info.plist',
			},
			
			'dependencies':
            [
                '../thirdparty/libopenssl/libopenssl.gyp:revsecurity_built',
                '../revpdfprinter/revpdfprinter.gyp:external-revpdfprinter',
				'kernel-development.gyp:kernel-development',
				'encode_environment_stack',
				'engine-common.gyp:security-community',
				'lcb-modules.gyp:engine_lcb_modules',
			],
			
			'sources':
			[
				'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
				'rsrc/development.rc',
				'>@(builtin_lcb_modules)',
			],

			'conditions':
			[
				[
					'OS != "win" and OS != "android"',
					{
						'sources':
						[
							'src/main.cpp',
						],
						'include_dirs':
						[
							'../libfoundation/include',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'mac_bundle_resources':
						[
							'rsrc/LiveCode.icns',
							'rsrc/LiveCodeDoc.icns',
                        ],
                        
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/LiveCode-Community.app/Contents/MacOS',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/revsecurity.dylib',
                                    '<(PRODUCT_DIR)/revpdfprinter.bundle',
                                ],
                            },
                        ],
					},
				],
				[
					'mobile != 0',
					{
						'type': 'none',
						'mac_bundle': 0,
					},
				],
			],
			
			'msvs_settings':
			{
				'VCManifestTool':
				{
					'AdditionalManifestFiles': '$(ProjectDir)..\\..\\..\\engine\\src\\engine.manifest',
				},
			},
			
			# Visual Studio debugging settings
			'run_as':
			{
				'action': [ '<(PRODUCT_DIR)/<(_product_name).exe' ],
				'environment':
				{
					'REV_TOOLS_PATH' : '$(ProjectDir)..\\..\\..\\ide',
				},
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(app_bundle_suffix)' ],
				},
			},
		},
		
		{
			'target_name': 'ios-standalone-executable',
			'type': 'none',
			
			'dependencies':
			[
				'standalone',
			],

			'conditions':
			[
				[
					'OS != "win" and OS != "android"',
					{
						'sources':
						[
							'src/main.cpp',
						],
						'include_dirs':
						[
							'../libfoundation/include',
						],
					},
				],
				[
					'OS == "ios"',
					{
						'actions':
						[
							{
								'action_name': 'bind-output',
								'message': 'Bind output',
								
								'inputs':
								[
									'<(PRODUCT_DIR)/standalone-mobile-lib-community.lcext',
								],
								
								'outputs':
								[
									'<(PRODUCT_DIR)/standalone-mobile-community.ios-engine',
								],
								
								'action':
								[
									'./bind-ios-standalone.sh',
									'<@(_inputs)',
									'<@(_outputs)',
								],
							},
						],
					},
				],
			],
		},
	],
	
	'conditions':
	[
		[
			'OS == "linux"',
			{
				'targets':
				[
					{
						'target_name': 'create_linux_stubs',
						'type': 'none',
												
						'actions':
						[
							{
								'action_name': 'linux_library_stubs',
								'inputs':
								[
									'../util/weak_stub_maker.pl',
									'src/linux.stubs',
								],
								'outputs':
								[
									'<(SHARED_INTERMEDIATE_DIR)/src/linux.stubs.cpp',
								],
								
								'action':
								[
									'<@(perl)',
									'../util/weak_stub_maker.pl',
									'src/linux.stubs',
									'<@(_outputs)',
								],
							},
						],
					},
				],
			}
		],
		[
			'OS == "emscripten"',
			{
				'targets':
				[
					{
						'target_name': 'javascriptify',
						'type': 'none',

						'dependencies':
						[
							'standalone',
						],

						'variables':
						{
							'version_suffix': '<(version_string)',
						},

						'actions':
						[
							{
								'action_name': 'genwhitelist',
								'message': 'Generating the Emterpreter whitelist',

								'inputs':
								[
									'../util/emscripten-genwhitelist.py',
									'<(PRODUCT_DIR)/standalone-community.bc',
									'src/em-whitelist.json',
									'src/em-blacklist.json',
								],

								'outputs':
								[
									'<(PRODUCT_DIR)/standalone-community-whitelist.json',
									'<(PRODUCT_DIR)/standalone-community-blacklist.json',
								],

								'action':
								[
									'../util/emscripten-genwhitelist.py',
									'--input',
									'<(PRODUCT_DIR)/standalone-community.bc',
									'--output',
									'<(PRODUCT_DIR)/standalone-community-whitelist.json',
									'<(PRODUCT_DIR)/standalone-community-blacklist.json',
									'--include',
									'src/em-whitelist.json',
									'--exclude',
									'src/em-blacklist.json',
								],
							},
							{
								'action_name': 'javascriptify',
								'message': 'Javascript-ifying the Emscripten engine',

								'inputs':
								[
									'../util/emscripten-javascriptify.py',
									'<(PRODUCT_DIR)/standalone-community.bc',
									'rsrc/emscripten-html-template.html',
									'<(PRODUCT_DIR)/standalone-community-whitelist.json',
									'src/em-preamble.js',
									'src/em-preamble-overlay.js',
									'src/em-util.js',
									'src/em-async.js',
									'src/em-dialog.js',
									'src/em-event.js',
									'src/em-surface.js',
									'src/em-system.js',
									'src/em-url.js',
									'src/em-standalone.js',
									'src/em-liburl.js',
									'src/em-dc.js',
								],

								'outputs':
								[
									'<(PRODUCT_DIR)/standalone-community-<(version_suffix).js',
									'<(PRODUCT_DIR)/standalone-community-<(version_suffix).html',
									'<(PRODUCT_DIR)/standalone-community-<(version_suffix).html.mem',
								],

								'action':
								[
									'../util/emscripten-javascriptify.py',
									'--input',
									'<(PRODUCT_DIR)/standalone-community.bc',
									'--output',
									'<(PRODUCT_DIR)/standalone-community-<(version_suffix).html',
									'--shell-file',
									'rsrc/emscripten-html-template.html',
									'--whitelist',
									'<(PRODUCT_DIR)/standalone-community-whitelist.json',
									'--pre-js',
									'src/em-preamble.js',
									'src/em-preamble-overlay.js',
									'--js-library',
									'src/em-util.js',
									'src/em-async.js',
									'src/em-dialog.js',
									'src/em-event.js',
									'src/em-surface.js',
									'src/em-system.js',
									'src/em-url.js',
									'src/em-standalone.js',
									'src/em-liburl.js',
									'src/em-dc.js',
								],
							},
						],
					},
				],
			},
		],
	],
}
