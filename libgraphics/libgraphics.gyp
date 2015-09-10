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
				'src/graphics-internal.h',
				
				'src/blur.cpp',
				'src/cachetable.cpp',
				'src/context.cpp',
				'src/coretext.cpp',
				'src/image.cpp',
				'src/legacyblendmodes.cpp',
				'src/legacygradients.cpp',
				'src/lnxtext.cpp',
				'src/mblandroidtext.cpp',
				#'src/osxtext.cpp', # UNUSED?
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
					'OS == "android"',
					{
						'dependencies':
						[
							'../prebuilt/libicu.gyp:libicu',
							'../thirdparty/libfreetype/libfreetype.gyp:libfreetype',
							'../thirdparty/libharfbuzz/libharfbuzz.gyp:libharfbuzz',
						],
						
						'sources':
						[
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
						'OS == "mac" or OS == "ios"',
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
