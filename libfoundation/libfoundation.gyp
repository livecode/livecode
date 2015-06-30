{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libFoundation',
			'type': 'static_library',
			
			'dependencies':
			[
				'../prebuilt/libicu.gyp:libicu',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'include/foundation.h',
				'include/foundation-auto.h',
				'include/foundation-bidi.h',
				'include/foundation-inline.h',
				'include/foundation-locale.h',
				'include/foundation-objc.h',
				'include/foundation-stdlib.h',
				'include/foundation-string.h',
				'include/foundation-text.h',
				'include/foundation-unicode.h',
				
				'src/foundation-private.h',
				'src/foundation-unicode-private.h',
				
				'src/foundation-array.cpp',
				'src/foundation-bidi.cpp',
				'src/foundation-core.cpp',
				'src/foundation-data.cpp',
				'src/foundation-debug.cpp',
				'src/foundation-error.cpp',
				'src/foundation-list.cpp',
				'src/foundation-locale.cpp',
				'src/foundation-name.cpp',
				'src/foundation-nativechars.cpp',
				'src/foundation-number.cpp',
				'src/foundation-set.cpp',
				'src/foundation-stream.cpp',
				'src/foundation-string.cpp',
				'src/foundation-string-cf.cpp',
				'src/foundation-text.cpp',
				'src/foundation-unicode.cpp',
				'src/foundation-unicodechars.cpp',
				'src/foundation-value.cpp',
				'src/foundation-objc.mm',
			],
			
			'conditions':
			[
				[
					'OS != "mac" and OS != "ios"',
					{
						'sources!':
						[
							'src/foundation-string-cf.cpp',
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
								'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
							],
						},
					],
					[
						'OS == "android"',
						{
							'libraries':
							[
								'-llog',
							],
						},
					],
				],
			},
		},
	],
}
