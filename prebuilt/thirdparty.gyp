{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'thirdparty_prebuilt_dep',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'fetch.gyp:fetch',
			],

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
									'GENERATOR != "xcode"',
									{
										'library_dirs':
										[
											'lib/mac',
										],
									},
								],
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
									},
								]
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
						},
					],
					[
						'OS == "emscripten"',
						{
							'library_dirs':
							[
								'lib/emscripten/js',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_expat',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libexpat',
					'../thirdparty/libexpat/lib',
				],
			},

			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lexpat',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{	
										'libraries':
										[
											'-lexpat',
										],
									},
								],
							],
						}
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_z',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libz/include',
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
											'lib/mac/libz.a',
										],
									},
									{
										'libraries':
										[
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
								'lib/ios/$(SDK_NAME)/libz.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lz',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lz',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibz',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lz',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_gif',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libgif/include',
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
											'lib/mac/libgif.a',
										],
									},
									{
										'libraries':
										[
											'-lgif',
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
								'lib/ios/$(SDK_NAME)/libgif.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lgif',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lgif',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibgif',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lgif',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_jpeg',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libjpeg/include',
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
											'lib/mac/libjpeg.a',
										],
									},
									{
										'libraries':
										[
											'-ljpeg',
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
								'lib/ios/$(SDK_NAME)/libjpeg.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-ljpeg',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-ljpeg',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibjpeg',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-ljpeg',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_png',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_z',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libpng/include',
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
											'lib/mac/libpng.a',
										],
									},
									{
										'libraries':
										[
											'-lpng',
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
								'lib/ios/$(SDK_NAME)/libpng.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lpng',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lpng',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibpng',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lpng',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_skia',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_z',
				'thirdparty_prebuilt_gif',
				'thirdparty_prebuilt_png',
				'thirdparty_prebuilt_jpeg',
				'thirdparty_prebuilt_expat',
				'thirdparty_prebuilt_freetype',
				'thirdparty_prebuilt_fontconfig',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
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
				],

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
											'lib/mac/libskia.a',
											'lib/mac/libskia_opt_none.a',
											'lib/mac/libskia_opt_arm.a',
											'lib/mac/libskia_opt_sse2.a',
											'lib/mac/libskia_opt_sse3.a',
											'lib/mac/libskia_opt_sse41.a',
											'lib/mac/libskia_opt_sse42.a',
											'lib/mac/libskia_opt_avx.a',
											'lib/mac/libskia_opt_hsw.a',
										],
									},
									{
										'libraries':
										[
											'-lskia',
											'-lskia_opt_none',
											'-lskia_opt_arm',
											'-lskia_opt_sse2',
											'-lskia_opt_sse3',
											'-lskia_opt_sse41',
											'-lskia_opt_sse42',
											'-lskia_opt_avx',
											'-lskia_opt_hsw',
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
								'lib/ios/$(SDK_NAME)/libskia.a',
								'lib/ios/$(SDK_NAME)/libskia.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_none.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_arm.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse2.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse3.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse41.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_sse42.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_avx.a',
								'lib/ios/$(SDK_NAME)/libskia_opt_hsw.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lskia',
								'-lskia_opt_none',
								'-lskia_opt_arm',
								'-lskia_opt_sse2',
								'-lskia_opt_sse3',
								'-lskia_opt_sse41',
								'-lskia_opt_sse42',
								'-lskia_opt_avx',
								'-lskia_opt_hsw',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lskia',
											'-lskia_opt_none',
											'-lskia_opt_arm',
											'-lskia_opt_sse2',
											'-lskia_opt_sse3',
											'-lskia_opt_sse41',
											'-lskia_opt_sse42',
											'-lskia_opt_avx',
											'-lskia_opt_hsw',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibskia',
								'-llibskia_opt_none',
								'-llibskia_opt_arm',
								'-llibskia_opt_sse2',
								'-llibskia_opt_sse3',
								'-llibskia_opt_sse41',
								'-llibskia_opt_sse42',
								'-llibskia_opt_avx',
								'-llibskia_opt_hsw',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lskia',
								'-lskia_opt_none',
								'-lskia_opt_arm',
								'-lskia_opt_sse2',
								'-lskia_opt_sse3',
								'-lskia_opt_sse41',
								'-lskia_opt_sse42',
								'-lskia_opt_avx',
								'-lskia_opt_hsw',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_harfbuzz',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libharfbuzz/src',
				],
			},

			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											# Hack to put licuuc after lharfbuzz in library list
											# to force the linking order we need
											'-lharfbuzz -licuuc',
										],
									},
								],
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lharfbuzz',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_freetype',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_z',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libfreetype/include',
				],
			},

			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[	
								'-lfreetype',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lfreetype',
										],
									},
								],
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lfreetype',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_fontconfig',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libfontconfig/include',
				],
			},

			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[	
								'-lfontconfig',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_cairo',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_z',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libcairo/src',
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
											'lib/mac/libcairo.a',
										],
									},
									{
										'libraries':
										[
											'-lcairo',
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
								'lib/ios/$(SDK_NAME)/libcairo.a',
							],
						},
					],
					[
						'toolset_os == "mac" or toolset_os == "ios"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lcairo',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lcairo',
							],
						},
					],
					[
						'toolset_os == "win"',
						{						
							'libraries':
							[
								'-lgdi32',
								'-luser32',
								'-lmsimg32',
								'-llibcairo',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_pcre',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libpcre/include',
				],

				'defines':
				[
					'PCRE_STATIC=1',
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
										],
									},
									{
										'libraries':
										[
											'-lpcre',
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
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lpcre',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lpcre',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibpcre',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lpcre',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_xml',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libxml/include',
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
											'lib/mac/libxml.a',
										],
									},
									{
										'libraries':
										[
											'-lxml',
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
								'lib/ios/$(SDK_NAME)/libxml.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lxml',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lxml',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibxml',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lxml',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_xslt',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libxslt/include',
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
											'lib/mac/libxslt.a',
										],
									},
									{
										'libraries':
										[
											'-lxslt',
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
								'lib/ios/$(SDK_NAME)/libxslt.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lxslt',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lxslt',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibxslt',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lxslt',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_ffi',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'all_dependent_settings':
			{
				'msvs_settings':
				{
					'VCLinkerTool':
					{
						# libffi is not safe exception handler compatible therefore nothing
						# linked to it is compatible either
						'ImageHasSafeExceptionHandlers': 'false',
					},
				},
			},

			'variables':
			{
				'conditions':
				[
					[
						'_toolset == "host"',
						{
							'toolset_os': '<(host_os)',
							'toolset_arch': '<(host_arch)',
						},
						{
							'toolset_os': '<(OS)',
							'toolset_arch': '<(target_arch)',
						},
					],
				],

				'libffi_public_headers_darwin_osx_dir':
				[
					'../thirdparty/libffi/include_darwin'
				],
				
				'libffi_public_headers_darwin_ios_dir':
				[
					'../thirdparty/libffi/git_master/darwin_ios/include',
					'../thirdparty/libffi/git_master/darwin_common/include',
				],
				
				'libffi_public_headers_win32_dir':
				[
					'../thirdparty/libffi/include_win32',
				],

				'libffi_public_headers_win64_dir':
				[
					'../thirdparty/libffi/include_win64',
				],
				
				'libffi_public_headers_linux_x86_dir':
				[
					'../thirdparty/libffi/include_linux/x86',
				],
				
				'libffi_public_headers_linux_x86_64_dir':
				[
					'../thirdparty/libffi/include_linux/x86_64',
				],

				'libffi_public_headers_linux_arm64_dir':
				[
					'../thirdparty/libffi/include_linux/arm64',
				],
				
				'libffi_public_headers_android_dir':
				[
					'../thirdparty/libffi/include_android',
				],
			},

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'<@(_platform_include_dirs)',
				],
			},

			'conditions':
			[
				[
					'toolset_os == "mac"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_darwin_osx_dir)',
						],
					},
				],
				[
					'toolset_os == "ios"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_darwin_ios_dir)',
						],
					},
				],
				[
					'toolset_os == "linux"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_darwin_osx_dir)',
						],
					},
				],
				[
					'toolset_os == "win"',
					{							
						'conditions':
						[
							[
								'toolset_arch == "x86"',
								{
									'platform_include_dirs':
									[
										'<@(libffi_public_headers_win32_dir)',
									],
								},
								{
									'platform_include_dirs':
									[
										'<@(libffi_public_headers_win64_dir)',
									],
								},
							],
						],
					},
				],
				[
					'(toolset_os == "linux" or toolset_os == "android") and toolset_arch == "x86"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_linux_x86_dir)',
						],
					},
				],
				[
					'(toolset_os == "linux" or toolset_os == "android") and toolset_arch == "x86_64"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_linux_x86_64_dir)',
						],
					},
				],
				[
					'toolset_os in ("linux", "android") and toolset_arch in ("armv6", "armv6hf", "armv7")',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_android_dir)',
						],
					},
				],
				[
					'toolset_os in ("linux", "android") and toolset_arch == "arm64"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_linux_arm64_dir)',
						],
					},
				],
				[
					'toolset_os == "emscripten"',
					{
						'platform_include_dirs':
						[
							'<@(libffi_public_headers_linux_x86_dir)',
						],
					},
				],
			],

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
											'lib/mac/libffi.a',
										],
									},
									{
										'libraries':
										[
											'-lffi',
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
								'lib/ios/$(SDK_NAME)/libffi.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lffi',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lffi',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibffi',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lffi',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_zip',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libzip/include',
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
											'lib/mac/libzip.a',
										],
									},
									{
										'libraries':
										[
											'-lzip',
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
								'lib/ios/$(SDK_NAME)/libzip.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lzip',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lzip',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibzip',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lzip',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_iodbc',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libiodbc/include',
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
											'lib/mac/libiodbc.a',
										],
									},
									{
										'libraries':
										[
											'-liodbc',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-liodbc',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_pq',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libpq/include',
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
											'lib/mac/libpq.a',
										],
									},
									{
										'libraries':
										[
											'-lpq',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lpq',
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-ladvapi32',
								'-lsecur32',
								'-lshell32',
								'-lwldap32',
								'-lws2_32',
								'-lwsock32',
								'-llibpq',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_mysql',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_z',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libmysql/include',
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
											'lib/mac/libmysql.a',
										],
									},
									{
										'libraries':
										[
											'-lmysql',
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
								'lib/ios/$(SDK_NAME)/libmysql.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lmysql',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lmysql',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-ladvapi32',
								'-luser32',
								'-llibmysql',
							],
						},
					],
				],
			},
		},
		{
			'target_name': 'thirdparty_prebuilt_sqlite',
			'type': 'none',

			'toolsets': ['host','target'],

			'dependencies':
			[
				'thirdparty_prebuilt_dep',
			],

			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'../thirdparty/libsqlite/include',
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
											'lib/mac/libsqlite.a',
										],
									},
									{
										'libraries':
										[
											'-lsqlite',
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
								'lib/ios/$(SDK_NAME)/libsqlite.a',
							],
						},
					],
					[
						'toolset_os == "linux"',
						{
							'libraries':
							[
								'-lsqlite',
							],
						},
					],
					[
						'toolset_os == "android"',
						{
							'conditions':
							[
								[
									'OS == "android"',
									{
										'libraries':
										[
											'-lsqlite',
										],
									},
								],
							],
						},
					],
					[
						'toolset_os == "win"',
						{							
							'libraries':
							[
								'-llibsqlite',
							],
						},
					],
					[
						'OS == "emscripten"',
						{
							'libraries':
							[
								'-lsqlite',
							],
						},
					],
				],
			},
		},
	],
}
