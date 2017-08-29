{
	'includes':
	[
		'../common.gypi',
		'prebuilt-common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libicu',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

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
								'-ldl',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'library_dirs':
							[
								'lib/android/<(target_arch)',
							],
							
							'libraries':
							[
								'-licui18n',
								'-licuio',
								'-licuuc',
								'-lstdc++',
								'-lm',
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
							],
						},
					],
				],
			},
		},

		{
			'target_name': 'encode_icu_data',
			'type': 'none',
			
			'toolsets': ['host'],

			'dependencies':
			[
				'libicu',
			],
			
			'actions':
			[
				{
					'action_name': 'encode_icu_data',
					'inputs':
					[
						'../util/encode_data.pl',
						'>(prebuilt_share_dir)/icudata.dat',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/icudata.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/encode_data.pl',
						'>(prebuilt_share_dir)/icudata.dat',
						'<@(_outputs)',
						# Really nasty hack to prevent this from being treated as a path
						'$(this_is_an_undefined_variable)s_icudata',
					],
				},
			],
		},
		
		{
			'target_name': 'minimal_icu_data',
			'type': 'none',
			
			'toolsets': ['host'],

			'dependencies':
			[
				'libicu',
			],
			
			'actions':
			[
				{
					'action_name': 'list_icu_data',
					'inputs':
					[
						'>(prebuilt_bin_dir)/icupkg',
						'>(prebuilt_share_dir)/icudata.dat',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-full-list.txt',
					],
					'action':
					[
						'>(prebuilt_bin_dir)/icupkg',
						'--list',
						'>(prebuilt_share_dir)/icudata.dat',
						'--auto_toc_prefix',
						'--outlist',
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-full-list.txt',
					],
				},
				
				{
					'action_name': 'gen_icu_data_remove_list',
					'inputs':
					[
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-full-list.txt',
						'rsrc/icudata-minimal-list.txt',
					],
					'outputs':
					[
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
					'action':
					[
						'../util/remove_matching.py',
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-full-list.txt',
						'rsrc/icudata-minimal-list.txt',
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
				},
				
				{
					'action_name': 'minimal_icu_data',
					'inputs':
					[
						'>(prebuilt_bin_dir)/icupkg',
						'>(prebuilt_share_dir)/icudata.dat',
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
					],
					
					'action':
					[
						'>(prebuilt_bin_dir)/icupkg',
						'--remove',
						'>(SHARED_INTERMEDIATE_DIR)/data/icudata-remove-list.txt',
						'--auto_toc_prefix',
						'>(prebuilt_share_dir)/icudata.dat',
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
					],
				},
			],
		},

		{
			'target_name': 'encode_minimal_icu_data',
			'type': 'none',
			
			'toolsets': ['host'],

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
						'../util/encode_data.pl',
						'<(SHARED_INTERMEDIATE_DIR)/data/icudata-minimal.dat',
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
