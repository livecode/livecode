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
							'library_dirs':
							[
								'lib/android/<(target_arch)',
							],
							
							'libraries':
							[
								'-licui18n',
								'-licuio',
								'-licuuc',
								'-licudata',
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
								'-licudata',
							],
						},
					],
				],
			},
		},
	],
}
