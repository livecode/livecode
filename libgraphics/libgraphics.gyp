{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libGraphics',
			'type': 'static_library',

			'toolsets': ['host', 'target'],
			
			'dependencies':
			[
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_gif',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_png',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_jpeg',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_skia',
				'../libfoundation/libfoundation.gyp:libFoundation',
			],

			'conditions':
			[
				[
					'OS in ("emscripten", "android")',
					{
						'dependencies':
						[
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_freetype',
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_harfbuzz',
							'../prebuilt/libicu.gyp:libicu',
						],
					},
				],
			],		
	
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'include/graphics.h',

				'src/graphics-internal.h',
				
				'src/blur.cpp',
				'src/cachetable.cpp',
				'src/context.cpp',
				'src/coretext-skia.cpp',
				'src/directwrite-skia.cpp',
				'src/image.cpp',
				'src/lnxtext.cpp',
				'src/harfbuzztext.cpp',
				'src/hb-sk.cpp',
				'src/path.cpp',
				'src/region.cpp',
				'src/spread.cpp',
				'src/utils.cpp',
				'src/SkStippleMaskFilter.cpp',
				'src/legacygradients.cpp',
				'src/drawing.cpp',
			],

			'sources!':
			[
				'src/hb-sk.cpp',
			],
			
			'target_conditions':
			[
				[
					'toolset_os != "mac" and toolset_os != "ios"',
					{
						'sources!':
						[
							'src/coretext-skia.cpp',
						],
					},
				],
				[
					'toolset_os != "win"',
					{
						'sources!':
						[
							'src/directwrite-skia.cpp',
						],
					},
				],
				[
					'toolset_os != "android" and toolset_os != "emscripten"',
					{
						'sources!':
						[
							'src/harfbuzztext.cpp',
						],
					},
				],
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
			
			'link_settings':
			{
				'conditions':
				[
                    [
                        'OS == "mac" and target_sdk == "macosx10.6"',
                        {
                            'libraries':
                            [
                                '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
                            ],
                        },
                    ],
                    [
                        'OS == "mac" and target_sdk != "macosx10.6"',
                        {
                            'libraries':
                            [
                                '$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
                                '$(SDKROOT)/System/Library/Frameworks/CoreText.framework',
                            ],
                        },
                    ],
					[
						'OS == "ios"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreText.framework',
							],
						},
					],
				],
			},
		},
	],
}
