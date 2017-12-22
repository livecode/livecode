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
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../thirdparty/libskia/libskia.gyp:libskia',
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
			
			'conditions':
			[
				[
					'OS in ("emscripten", "android")',
					{
						'dependencies':
						[
							'../prebuilt/libicu.gyp:libicu',
							'../thirdparty/libfreetype/libfreetype.gyp:libfreetype',
							'../thirdparty/libharfbuzz/libharfbuzz.gyp:libharfbuzz',
						],
					},
				],
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
							'src/hb-sk.cpp',
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
