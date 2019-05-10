{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'thirdparty_prebuilt',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

			'direct_dependent_settings':
			{
				'defines':
				[
					'PCRE_STATIC=1',
				],
				
				'include_dirs':
				[
					'../thirdparty/libexpat',
					'../thirdparty/libexpat/lib',
					'../thirdparty/libgif/include',
					'../thirdparty/libharfbuzz/src',
					'../thirdparty/libjpeg/include',
					'../thirdparty/libpcre/include',
					'../thirdparty/libpng/include',
					'../thirdparty/libz/include',
					'../thirdparty/libskia/include/config',
					'../thirdparty/libskia/include/core',
					'../thirdparty/libskia/include/device',
					'../thirdparty/libskia/include/effects',
					'../thirdparty/libskia/include/images',
					'../thirdparty/libskia/include/pathops',
					'../thirdparty/libskia/include/pdf',
					'../thirdparty/libskia/include/pipe',
					'../thirdparty/libskia/include/ports',
					'../thirdparty/libskia/include/svg',
					'../thirdparty/libskia/include/text',
					'../thirdparty/libskia/include/utils',
					# Needed for some legacy methods
					'../thirdparty/libskia/src/core',
					# Needed for directwrite text rendering on Windows
					'../thirdparty/libskia/src/ports',
					'../thirdparty/libskia/include/private',
					'../thirdparty/libskia/src/utils/win',

					'../thirdparty/libfreetype/include',
				],
			},
			
			'all_dependent_settings':
			{
				'defines':
				[
					# Disable Skia debugging
					'SK_RELEASE',
				
					# We use deprecated Skia features
					'SK_SUPPORT_LEGACY_CANVAS_IS_REFCNT',
					'SK_SUPPORT_LEGACY_GETTOPDEVICE',
					'SK_SUPPORT_LEGACY_ACCESSBITMAP',
					'SK_SUPPORT_LEGACY_CLIP_REGIONOPS',
					'SK_SUPPORT_LEGACY_GETDEVICE',
					
					# Disable GPU support
					'SK_SUPPORT_GPU=0',
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
											'lib/mac/libpcre.a',
											# libskia depends on libgif, libjpeg, libpng, libz
											'lib/mac/libskia.a',
											'lib/mac/libskia_opt_none.a',
											'lib/mac/libskia_opt_arm.a',
											'lib/mac/libskia_opt_sse2.a',
											'lib/mac/libskia_opt_sse3.a',
											'lib/mac/libskia_opt_sse41.a',
											'lib/mac/libskia_opt_sse42.a',
											'lib/mac/libskia_opt_avx.a',
											'lib/mac/libskia_opt_hsw.a',
											'lib/mac/libgif.a',
											'lib/mac/libjpeg.a',
											# libpng depends on libz
											'lib/mac/libpng.a',
											'lib/mac/libz.a',
										],
									},
									{
										'library_dirs':
										[
											'lib/mac',
										],
										
										'libraries':
										[
											'-lpcre',
											# libskia depends on libgif, libjpeg, libpng, libz
											'-lskia',
											'-lskia_opt_none',
											'-lskia_opt_arm',
											'-lskia_opt_sse2',
											'-lskia_opt_sse3',
											'-lskia_opt_sse41',
											'-lskia_opt_sse42',
											'-lskia_opt_avx',
											'-lskia_opt_hsw',
											'-lgif',
											'-ljpeg',
											# libpng depends on libz
											'-lpng',
											'-lz',
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
								'lib/ios/$(SDK_NAME)/libpcre.a',
								'lib/ios/$(SDK_NAME)/libskia.a',
								# libskia depends on libgif, libjpeg, libpng, libz
								'lib/ios/$(SDK_NAME)/libskia.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_none.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_arm.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse2.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse3.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse41.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse42.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_avx.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_hsw.a',
								'lib/ios/$(SDK_NAME)/libgif.a',
								'lib/ios/$(SDK_NAME)/libjpeg.a',
								# libpng depends on libz
								'lib/ios/$(SDK_NAME)/libpng.a',
								'lib/ios/$(SDK_NAME)/libz.a',
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
								'-lpcre',
								# libskia depends on libgif, libjpeg, libpng, libz
								'-lskia',
								'-lskia_opt_none',
								'-lskia_opt_arm',
								'-lskia_opt_sse2',
								'-lskia_opt_sse3',
								'-lskia_opt_sse41',
								'-lskia_opt_sse42',
								'-lskia_opt_avx',
								'-lskia_opt_hsw',
								'-lgif',
								'-ljpeg',
								# libpng depends on libz
								'-lpng',
								'-lz',
								
								#'-lGL',
								'-lfreetype',
								'-lfontconfig',
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
											# *ANDROID* libharfbuzz depends on libicu
											'-lharfbuzz',
											'-lpcre',
											# *ANDROID* libskia depends on libexpat, libfreetype
											# libskia depends on libgif, libjpeg, libpng, libz
											'-lskia',
											'-lskia_opt_none',
											'-lskia_opt_arm',
											'-lskia_opt_sse2',
											'-lskia_opt_sse3',
											'-lskia_opt_sse41',
											'-lskia_opt_sse42',
											'-lskia_opt_avx',
											'-lskia_opt_hsw',
											'-lexpat',
											# libfreetype depends on libz
											'-lfreetype',
											'-lgif',
											'-ljpeg',
											# libpng depends on libz
											'-lpng',
											'-lz',
											
											#'-lEGL',
											#'-lGLESv2',
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
								'unpacked/Thirdparty/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/lib',
							],
							
							'libraries':
							[
								'-llibpcre',
								# libskia depends on libgif, libjpeg, libpng, libz
								'-llibskia',
								'-llibskia_opt_none',
								'-llibskia_opt_arm',
								'-llibskia_opt_sse2',
								'-llibskia_opt_sse3',
								'-llibskia_opt_sse41',
								'-llibskia_opt_sse42',
								'-llibskia_opt_avx',
								'-llibskia_opt_hsw',
								'-llibgif',
								'-llibjpeg',
								# libpng depends on libz
								'-llibpng',
								'-llibz',
								
								#'-lOpenGL32.lib'
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
								# *EMSCRIPTEN* libharfbuzz depends on libicu
								'-lharfbuzz',
								'-lpcre',
								# *EMSCRIPTEN* libskia depends on libfreetype
								# libskia depends on libgif, libjpeg, libpng, libz
								'-lskia',
								'-lskia_opt_none',
								'-lskia_opt_arm',
								'-lskia_opt_sse2',
								'-lskia_opt_sse3',
								'-lskia_opt_sse41',
								'-lskia_opt_sse42',
								'-lskia_opt_avx',
								'-lskia_opt_hsw',
								# libfreetype depends on libz
								'-lfreetype',
								'-lgif',
								'-ljpeg',
								# libpng depends on libz
								'-lpng',
								'-lz',
							],
						},
					],
				],
			},
		},
	],
}
