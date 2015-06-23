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
			
			'toolsets': ['host','target'],
			
			'product_prefix': '',
			'product_name': 'libFoundation',
			
			'dependencies':
			[
				'../prebuilt/libicu.gyp:libicu',
				'../thirdparty/libffi/libffi.gyp:libffi',
				'../thirdparty/libz/libz.gyp:libz',
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
				'include/foundation-chunk.h',
				'include/foundation-filters.h',
				'include/foundation-inline.h',
				'include/foundation-locale.h',
				'include/foundation-math.h',
				'include/foundation-objc.h',
				'include/foundation-stdlib.h',
				'include/foundation-string.h',
				'include/foundation-system.h',
				'include/foundation-text.h',
				'include/foundation-unicode.h',
				'include/system-commandline.h',
				'include/system-file.h',
				'include/system-init.h',
				'include/system-random.h',
				'include/system-stream.h',
				
				'src/foundation-private.h',
				'src/foundation-unicode-private.h',
				'src/system-private.h',
				
				'src/foundation-array.cpp',
				'src/foundation-bidi.cpp',
				'src/foundation-chunk.cpp',
				'src/foundation-core.cpp',
				'src/foundation-custom.cpp',
				'src/foundation-data.cpp',
				'src/foundation-debug.cpp',
				'src/foundation-error.cpp',
				'src/foundation-filters.cpp',
				'src/foundation-foreign.cpp',
				'src/foundation-handler.cpp',
				'src/foundation-list.cpp',
				'src/foundation-locale.cpp',
				'src/foundation-math.cpp',
				'src/foundation-name.cpp',
				'src/foundation-nativechars.cpp',
				'src/foundation-number.cpp',
				'src/foundation-pickle.cpp',
				'src/foundation-proper-list.cpp',
				'src/foundation-record.cpp',
				'src/foundation-set.cpp',
				'src/foundation-stream.cpp',
				'src/foundation-string.cpp',
				'src/foundation-string-cf.cpp',
				'src/foundation-text.cpp',
				'src/foundation-typeconvert.cpp',
				'src/foundation-typeinfo.cpp',
				'src/foundation-unicode.cpp',
				'src/foundation-unicodechars.cpp',
				'src/foundation-value.cpp',
				'src/foundation-objc.mm',
				'src/system-commandline.cpp',
				'src/system-file.cpp',
				'src/system-file-posix.cpp',
				'src/system-file-w32.cpp',
				'src/system-init.cpp',
				'src/system-random.cpp',
				'src/system-stream.cpp',
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
				[
					'OS == "win"',
					{
						'sources/':
						[
							['exclude', '.*-posix\\.cpp$'],
						],
					},
					{
						'sources/':
						[
							['exclude', '.*-w32\\.cpp$'],
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
			
			'target_conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'libFoundation->(_toolset)',
					},
				],
			],
			
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
								'-lm',
							],
						},
					],
				],
			},
		},
	],
}
