{
	'includes':
	[
		'../common.gypi',
	],
	
	'target_defaults':
	{
		'conditions':
		[
			[
				'host_os == "mac"',
				{
					'variables':
					{
						'prebuilt_icu_bin_dir': 'bin/mac',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				# Workaround x86 linux builder identifying (via uname -m) as x86_64:
				#   Use target arch executables if building on linux for linux
				'host_os == "linux" and OS == "linux" and cross_compile == 0',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_icu_bin_dir': 'bin/linux/<(target_arch)',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				'host_os == "linux" and (OS != "linux" or cross_compile != 0)',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_icu_bin_dir': 'bin/linux/<(host_arch)',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				'host_os == "win"',
				{
					'variables':
					{
						# Hack required due to GYP failure / refusal to treat this as a path
						'prebuilt_icu_bin_dir': '$(ProjectDir)../../../prebuilt/unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/bin',
						'prebuilt_icu_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
					},
				},
			],
		],
	},
	
	'targets':
	[
		{
			'target_name': 'libicu',
			'type': 'none',
			
			'toolsets': ['host', 'target'],
			
			'dependencies':
			[
				'fetch.gyp:fetch',
				'libicu_include',
				'libicu_link',
			],
		},
		{
			'target_name': 'libicu_include',
			'type': 'none',

			'toolsets': ['host','target'],

			# Needs to be all dependents as used by the HarfBuzz public headers
			'all_dependent_settings':
			{
				'defines':
				[
					# Ensure that symbols are referenced in the right way for a static lib
					'U_STATIC_IMPLEMENTATION=1',
				],

				'conditions':
				[
					[
						'OS == "win"',
						{
							'include_dirs':
							[
								'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
							],
						},
					],
					[
						'OS != "win"',
						{
							'include_dirs':
							[
								'include',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'libicu_link',
			'type': 'none',

			'toolsets': ['host','target'],

			'link_settings':
			{	
				'target_conditions':
				[
					[
						'toolset_os == "mac"',
						{
							'conditions':
							[
								[
									'GENERATOR == "xcode"',
									{
										'libraries':
										[
											'lib/mac/libicui18n.a',
											'lib/mac/libicuio.a',
											'lib/mac/libicutu.a',
											'lib/mac/libicuuc.a',
											'lib/mac/libicudata.a',
										],
									},
									{
										'library_dirs':
										[
											'lib/mac',
										],
										
										'libraries':
										[
											'-licui18n',
											'-licuio',
											'-licutu',
											'-licuuc',
											'-licudata',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "ios"',
						{
							'libraries':
							[
								'lib/ios/$(SDK_NAME)/libicui18n.a',
								'lib/ios/$(SDK_NAME)/libicuio.a',
								'lib/ios/$(SDK_NAME)/libicuuc.a',
								'lib/ios/$(SDK_NAME)/libicudata.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'library_dirs':
							[
								'lib/linux/>(toolset_arch)',
							],
							
							'libraries':
							[
								'-licui18n',
								'-licuio',
								'-licutu',
								'-licuuc',
								'-licudata',
								'-ldl',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'conditions':
							[
								[
									'OS == "android"',
									{
										'library_dirs':
										[
											'lib/android/<(target_arch)/<(android_subplatform)',
										],
										
										'libraries':
										[
											'-licui18n',
											'-licuio',
											'-licuuc',
											'-licudata',
											'-lstdc++',
											'-lm',
											'-latomic',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{
							'library_dirs':
							[
								'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
							],
							
							'libraries':
							[
								'-lsicuin',
								'-lsicuio',
								'-lsicutu',
								'-lsicuuc',
								'-lsicudt',
								
								# ICU dependencies
								'-ladvapi32',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'library_dirs':
							[
								'lib/emscripten/js',
							],

							'libraries':
							[
								'-licui18n',
								'-licuio',
								'-licutu',
								'-licuuc',
								'-licudata',
							],
						},
					],
				],
			},
		},

		{
			'target_name': 'minimal_icu_data',
			'type': 'none',
			
			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'fetch.gyp:fetch#host',
			],
			
			'actions':
			[
				{
					'action_name': 'list_icu_data',
					'inputs':
					[
						'>(prebuilt_icu_share_dir)/icudt58l.dat',
					],
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/data/icudata-full-list.txt',
					],
					'action':
					[
						'>(prebuilt_icu_bin_dir)/icupkg',
						'--list',
						'>(prebuilt_icu_share_dir)/icudt58l.dat',
						'--auto_toc_prefix',
						'--outlist',
						'<(INTERMEDIATE_DIR)/data/icudata-full-list.txt',
					],
				},
				
				{
					'action_name': 'gen_icu_data_remove_list',
					'inputs':
					[
						'<(INTERMEDIATE_DIR)/data/icudata-full-list.txt',
						'rsrc/icudata-minimal-list.txt',
					],
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
					'action':
					[
						'python',
						'../util/remove_matching.py',
						'<(INTERMEDIATE_DIR)/data/icudata-full-list.txt',
						'rsrc/icudata-minimal-list.txt',
						'<(INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
				},
				
				{
					'action_name': 'minimal_icu_data',
					'inputs':
					[
						'<(INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
					],
					
					'action':
					[
						'>(prebuilt_icu_bin_dir)/icupkg',
						'--remove',
						'<(INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
						'--auto_toc_prefix',
						'>(prebuilt_icu_share_dir)/icudt58l.dat',
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
					],
				},
			],
		},

		{
			'target_name': 'encode_minimal_icu_data',
			'type': 'none',
			
			'toolsets': ['host', 'target'],

			'dependencies':
			[
				'minimal_icu_data',
			],
			
			'actions':
			[
				{
					'action_name': 'encode_minimal_icu_data',
					'inputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
						'../util/encode_data.pl',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/icudata-minimal.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/encode_data.pl',
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
						'<@(_outputs)',
						# Really nasty hack to prevent this from being treated as a path
						'$(this_is_an_undefined_variable)s_icudata',
					],
				},
			],
		},
		
	],
}
