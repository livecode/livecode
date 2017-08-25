{
	'includes':
	[
		'../common.gypi',
	],
	
	'target_defaults':
	{
		'variables':
		{
			'target_conditions':
			[
				[
					'toolset_os == "mac"',
					{
						'icu_library_dir': 'lib/mac',
						'icu_include_dir': 'include',
						'icu_share_dir': 'share/mac',
					},
				],
				[
					'toolset_os == "ios"',
					{
						'icu_library_dir': 'lib/ios/$(SDK_NAME)',
						'icu_include_dir': 'include',
						'icu_share_dir': 'share/ios/$(SDK_NAME)',
					},
				],
				[
					'toolset_os == "linux"',
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'icu_library_dir': 'lib/linux/>(toolset_arch)',
						'icu_include_dir': 'include',
						'icu_share_dir': 'share/linux/>(toolset_arch)',
					},
				],
				[
					'toolset_os == "android"',
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'icu_library_dir': 'lib/android/<(target_arch)',
						'icu_include_dir': 'include',
						'icu_share_dir': 'share/android/<(target_arch)',
					},
				],
				[
					'toolset_os == "win"',
					{
						'icu_library_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
						'icu_include_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/include',
						'icu_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
					},
				],
				[
					'OS == "emscripten"',
					{
						'icu_library_dir': 'lib/emscripten/js',
						'icu_include_dir': 'include',
						'icu_share_dir': 'share/emscripten/js',
					},
				],
				
			],
		},
	},
	
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
			
			'toolsets': ['host','target'],

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
						'>(icu_share_dir)/icudata.dat',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/icudata.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/encode_data.pl',
						'>(icu_share_dir)/icudata.dat',
						'<@(_outputs)',
						# Really nasty hack to prevent this from being treated as a path
						'$(this_is_an_undefined_variable)s_icudata',
					],
				},
			],
		},
	],
}
