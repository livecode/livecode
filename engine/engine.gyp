{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
		'kernel.gypi',
		'kernel-development.gypi',
		'kernel-installer.gypi',
		'kernel-standalone.gypi',
		'kernel-server.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'encode_version',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'encode_version',
					'inputs':
					[
						'../util/encode_version.pl',
						'../version',
						'include/revbuild.h.in',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/include/revbuild.h',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/encode_version.pl',
						'.',
						'<(SHARED_INTERMEDIATE_DIR)',
					],
				},
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'<(SHARED_INTERMEDIATE_DIR)/include',
				],
			},
		},
		
		{
			'target_name': 'quicktime_stubs',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'quicktime_stubs',
					'inputs':
					[
						'../util/weak_stub_maker.pl',
						'src/quicktime.stubs',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/quicktimestubs.mac.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/weak_stub_maker.pl',
						'src/quicktime.stubs',
						'<@(_outputs)',
					],
				},
			],
		},
		
		{
			'target_name': 'encode_environment_stack',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'encode_environment_stack',
					'inputs':
					[
						'../util/compress_data.pl',
						'src/Environment.rev',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/compress_data.pl',
						'src/Environment.rev',
						'<@(_outputs)',
						# Really nasty hack to prevent this from being treated as a path
						'$(this_is_an_undefined_variable)MCstartupstack',
					],
				},
			],
		},
		
		{
			'target_name': 'security-community',
			'type': 'static_library',
			
			'dependencies':
			[
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl',
				
				# Because our headers are so messed up...
				'../libcore/libcore.gyp:libCore',
				'../libgraphics/libgraphics.gyp:libGraphics',
			],
			
			'sources':
			[
				'<@(engine_security_source_files)',
			],
		},
		
		{
			'target_name': 'server',
			'type': 'executable',
			'product_name': 'server-community',
			
			'dependencies':
			[
				'kernel-server',
				
				'../libcore/libcore.gyp:libCore',
				'../libgraphics/libgraphics.gyp:libGraphics',
			],
			
			'sources':
			[
				'<@(engine_security_source_files)',
			],
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'product_name': 'Server-Community',
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
				'kernel-standalone',
				'security-community',
			],
			
			'sources':
			[
				'src/dummy.cpp',
				'rsrc/standalone.rc',
			],
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'product_name': 'Standalone-Community',
					},
				],
				[
					'OS == "ios"',
					{
						'product_name': 'standalone-mobile-community',
						'app_plist': 'rsrc/standalone-mobile-Info.plist',
					},
				],
				[
					# Use a linker script to add the project and payload sections to the Linux executable
					'OS == "linux"',
					{
						'ldflags':
						[
							'-T', '<(src_top_dir_abs)/engine/linux.link',
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
						'type': 'loadable_module',
						
						'ldflags':
						[
							# Helpful for catching build problems
							'-Wl,-no-undefined',
							
							'-Wl,-T,<(src_top_dir_abs)/engine/linux.link',
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
							'OS != "android"',
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
				'kernel-installer',
				'security-community',
			],
			
			'sources':
			[
				'src/dummy.cpp',
				'rsrc/installer.rc',
			],
			
			'conditions':
			[
				[
					'OS == "mac"',
					{
						'product_name': 'Installer',
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
							'-T', '<(src_top_dir_abs)/engine/linux.link',
						],
					},
				],
			],
			
			'msvs_settings':
			{
				'VCManifestTool':
				{
					'AdditionalManifestFiles': '$(SolutionDir)..\\engine\\src\\installer.manifest',
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
			'product_name': 'livecode-community',
			
			'includes':
			[
				'app-bundle-template.gypi',
			],
			
			'variables':
			{
				'app_plist': 'rsrc/Revolution-Info.plist',
			},
			
			'dependencies':
			[
				'kernel-development',
				'encode_environment_stack',
				'security-community',
				
				'../thirdparty/libopenssl/libopenssl.gyp:revsecurity',
			],
			
			'sources':
			[
				'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
				'rsrc/development.rc',
			],

			'conditions':
			[
				[
					'OS == "mac"',
					{
						'product_name': 'LiveCode-Community',

						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)/<(_product_name).app/Contents/MacOS',
								'files':
								[
									'<(PRODUCT_DIR)/revsecurity.dylib',
								],
							},
						],
					},
				],
				[
					'OS == "win"',
					{
						'product_name': 'engine-community',
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
					'AdditionalManifestFiles': '$(SolutionDir)..\\engine\\src\\engine.manifest',
				},
			},
			
			# Visual Studio debugging settings
			'run_as':
			{
				'action': [ '<(PRODUCT_DIR)/<(_product_name).exe' ],
				'environment':
				{
					'REV_TOOLS_PATH' : '$(SolutionDir)..\\ide',
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
			'target_name': 'standalone-mobile-lib-community',
			'type': 'none',
			
			'dependencies':
			[
				'kernel-standalone',
			],
			
			'conditions':
			[
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
									'<(PRODUCT_DIR)/libkernel.a',
									'<(PRODUCT_DIR)/libkernel-standalone.a',
								],
								
								'outputs':
								[
									'<(PRODUCT_DIR)/standalone-mobile-lib-community.lcext',
								],
								
								'action':
								[
									'./bind-ios-standalone.sh',
									'<@(_outputs)',
									'<@(_inputs)',
								],
							},
						],
					},
				],
			],
		},
	],
}
