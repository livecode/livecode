{
	'variables':
	{
		'module_name': 'libFoundation',
		'module_test_dependencies':
		[
			'libFoundation',
		],
		'module_test_sources':
		[
			'test/environment.cpp',
			'test/test_string.cpp',
			'test/test_typeconvert.cpp',
		],
	},


	'includes':
	[
		'../common.gypi',
		'../config/cpptest.gypi'
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
				'include/foundation-system.h',
				'include/foundation-text.h',
				'include/foundation-unicode.h',
				'include/system-commandline.h',
				'include/system-dylib.h',
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
				'src/foundation-number.cpp',
				'src/foundation-pickle.cpp',
				'src/foundation-proper-list.cpp',
				'src/foundation-record.cpp',
				'src/foundation-set.cpp',
				'src/foundation-stream.cpp',
				'src/foundation-string.cpp',
				'src/foundation-string-cf.cpp',
                'src/foundation-string-native.cpp.h',
				'src/foundation-text.cpp',
				'src/foundation-typeconvert.cpp',
				'src/foundation-typeinfo.cpp',
				'src/foundation-unicode.cpp',
				'src/foundation-unicodechars.cpp',
				'src/foundation-value.cpp',
				'src/foundation-objc.mm',
				'src/foundation-ffi-js.cpp',
				'src/system-commandline.cpp',
				'src/system-file.cpp',
				'src/system-file-posix.cpp',
				'src/system-file-w32.cpp',
				'src/system-dylib.cpp',
				'src/system-dylib-lnx.cpp',
				'src/system-dylib-mac.cpp',
				'src/system-dylib-w32.cpp',
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
							['exclude', '.*-mac\\.cpp$'],
							['exclude', '.*-lnx\\.cpp$'],
						],
					},
				],
				[
					'OS == "linux"', 
					{
						'sources/':
						[
							['exclude', '.*-w32\\.cpp$'],
							['exclude', '.*-mac\\.cpp$'],	
						],
					},
				],
				[
					'OS == "mac"',
					{
						'sources/':
						[
							['exclude', '.*-w32\\.cpp$'],
							['exclude', '.*-lnx\\.cpp$'],	
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
				[
					'toolset_os != "emscripten"',
					{
						'sources!':
						[
							'src/foundation-ffi-js.cpp',
						],
					},
				],
			],
			
			'link_settings':
			{
				'target_conditions':
				[
					[
						'toolset_os == "mac" or toolset_os == "ios"',
						{
							'conditions':
							[
								[
									'GENERATOR == "xcode"',
									{							
										'libraries':
										[
											'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
											'$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
										],
									},
									{
										'ldflags':
										[
											'-framework', 'CoreFoundation',
											'-framework', 'Foundation',
										],
									},
								],
							],
						},
					],
					[
						'OS == "android" and _toolset == "target"',
						{
							'libraries':
							[
								'-llog',
								'-lm',
							],
						},
					],
					[
						'OS == "win"',
						{
							'libraries':
							[
								'-loleaut32',
							],
						},
					],
				],
			},
		},
	],
}
