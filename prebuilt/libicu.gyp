{
	'includes':
	[
		'../common.gypi',
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
			
			'toolsets': ['host','target'],

			'dependencies':
			[
				'libicu',
			],
			
			'variables':
			{
				'target_conditions':
				[
					[
						'OS == "mac"',
						{
							'icu_share_dir': 'share/mac',
						},
					],
					[
						'OS == "ios"',
						{
							'icu_share_dir': 'share/ios/$(SDK_NAME)',
						},
					],
					[
						'OS == "linux"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'icu_share_dir': 'share/linux/>(toolset_arch)',
						},
					],
					[
						'OS == "android"',
						{
							# Gyp doesn't seem to handle non-absolute paths here properly...
							'icu_share_dir': 'share/android/<(target_arch)',
						},
					],
					[
						'OS == "win"',
						{
							'icu_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
						},
					],
					[
						'OS == "emscripten"',
						{
							'icu_share_dir': 'share/emscripten/js',
						},
					],
					
				],
			},
			
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
						'<(INTERMEDIATE_DIR)/src/icudata.cpp',
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
