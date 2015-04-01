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
						'encode_version.rev',
						'../version',
						'include/revbuild.h.in',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/include/revbuild.h',
					],
					
					'action':
					[
						'<(revolution_path)',
						'encode_version.rev',
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
						'../tools/weak_stub_maker.lc',
						'src/quicktime.stubs',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/quicktimestubs.mac.cpp',
					],
					
					'action':
					[
						'<(revolution_path)',
						'../tools/weak_stub_maker.lc',
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
						'compress_data.rev',
						'src/environment.rev',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
					],
					
					'action':
					[
						'<(revolution_path)',
						'compress_data.rev',
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
			],
			
			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},
		},
		
		{
			'target_name': 'standalone',
			
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
			],
		},
		
		{
			'target_name': 'installer',
			
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
			],
			
			'msvs_settings':
			{
				'VCManifestTool':
				{
					'AdditionalManifestFiles': '$(SolutionDir)..\\engine\\src\\installer.manifest',
				},
			},
		},
		
		{
			'target_name': 'development',
			
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
			],
			
			'msvs_settings':
			{
				'VCManifestTool':
				{
					'AdditionalManifestFiles': '$(SolutionDir)..\\engine\\src\\engine.manifest',
				},
			},
		},
	],
}
