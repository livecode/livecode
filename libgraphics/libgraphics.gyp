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
				'src/coretext.cpp',
				'src/image.cpp',
				'src/lnxtext.cpp',
				'src/harfbuzztext.cpp',
				'src/hb-sk.cpp',
				'src/path.cpp',
				'src/region.cpp',
				'src/spread.cpp',
				'src/utils.cpp',
				'src/w32text.cpp',
			],
			
			'conditions':
			[
				[
					'OS != "mac" and OS != "ios"',
					{
						'sources!':
						[
							'src/coretext.cpp',
						],
					},
				],
				[
					'OS != "android" and OS != "emscripten"',
					{
						'sources!':
						[
							'src/harfbuzztext.cpp',
							'src/hb-sk.cpp',
						],
					},
				],
				[
					'OS == "android" or OS == "emscripten"',
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
