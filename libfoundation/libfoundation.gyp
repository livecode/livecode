{
	'variables':
	{
		'module_name': 'libFoundation',
		'module_test_dependencies':
		[
			'libFoundation',
		],
        'module_test_include_dirs':
        [
            'include',
            'src',
        ],
		'module_test_sources':
		[
			'test/environment.cpp',
            'test/test_foreign.cpp',
			'test/test_hash.cpp',
            'test/test_memory.cpp',
            'test/test_name.cpp',
			'test/test_proper-list.cpp',
			'test/test_string.cpp',
			'test/test_typeconvert.cpp',
            'test/test_system-library.cpp',
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
				'../prebuilt/libicu.gyp:encode_minimal_icu_data',
                '../prebuilt/thirdparty.gyp:thirdparty_prebuilt_z',
				'../thirdparty/libffi/libffi.gyp:libffi',
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
                'include/foundation-span.h',
				'include/foundation-stdlib.h',
				'include/foundation-system.h',
				'include/foundation-text.h',
				'include/foundation-unicode.h',
				'include/system-commandline.h',
				'include/system-error.h',
				'include/system-file.h',
				'include/system-init.h',
                'include/system-library.h',
				'include/system-random.h',
				'include/system-stream.h',
				'include/system-info.h',
				
				'src/foundation-private.h',
				'src/foundation-unicode-private.h',
				'src/system-private.h',
				
				'src/foundation-array.cpp',
				'src/foundation-bidi.cpp',
                'src/foundation-chunk.cpp',
                'src/foundation-cf.cpp',
				'src/foundation-core.cpp',
				'src/foundation-custom.cpp',
				'src/foundation-data.cpp',
				'src/foundation-debug.cpp',
				'src/foundation-error.cpp',
				'src/foundation-filters.cpp',
				'src/foundation-foreign.cpp',
				'src/foundation-java.cpp',
				'src/foundation-java-private.cpp',
				'src/foundation-java-private.h',
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
				'src/system-error.cpp',
				'src/system-file.cpp',
				'src/system-file-posix.cpp',
				'src/system-file-w32.cpp',
                'src/system-init.cpp',
                'src/system-library.cpp',
                'src/system-library-android.hpp',
                'src/system-library-emscripten.hpp',
                'src/system-library-ios.hpp',
                'src/system-library-mac.hpp',
                'src/system-library-posix.hpp',
                'src/system-library-static.hpp',
                'src/system-library-linux.hpp',
                'src/system-library-w32.hpp',
				'src/system-random.cpp',
				'src/system-stream.cpp',
				'src/system-info.cpp',
				
				'<(SHARED_INTERMEDIATE_DIR)/src/icudata-minimal.cpp',
			],

			'actions':
			[
				{
					'action_name': 'generate_libfoundationjvm_stubs',
					'inputs':
					[
						'../util/weak_stub_maker.pl',
						'jvm.stubs',
					],
					'outputs':
					[
						'<(INTERMEDIATE_DIR)/src/libfoundationjvm.stubs.cpp',
					],

					'action':
					[
						'<@(perl)',
						'../util/weak_stub_maker.pl',
						'--foundation',
						'jvm.stubs',
						'<@(_outputs)',
					],
				},
			],
			
			'conditions':
			[
				[
					'OS != "mac" and OS != "ios"',
					{
                        'sources':
                        [
                            'src/foundation-objc-dummy.cpp',
                        ],
                    
						'sources!':
						[
							'src/foundation-cf.cpp',
                            'src/foundation-objc.mm',
						],
					},
				],
				[
					'OS != "win"',
					{
						'sources/':
						[
							['exclude', '.*-w32\\.cpp$'],
						],
					},
                    {
                        'sources/':
                        [
                            ['exclude', '.*-posix\\.cpp$'],
                        ],
                    },
                ],
                [
                    'OS != "mac"',
                    {
                        'sources/':
                        [
                            ['exclude', '.*-mac\\.cpp$'],
                        ],
                    },
                ],
                [
                    'OS != "ios"',
                    {
                        'sources/':
                        [
                            ['exclude', '.*-ios\\.cpp$'],
                        ],
                    },
                ],
                [
                    'OS != "linux"',
                    {
                        'sources/':
                        [
                            ['exclude', '.*-linux\\.cpp$'],
                        ],
                    },
                ],
                [
                    'OS != "android"',
                    {
                        'sources/':
                        [
                            ['exclude', '.*-android\\.cpp$'],
                        ],
                    }
                ],
                
                # Set java-related defines and includes
                [
					'host_os == "mac" and OS != "ios" and OS != "emscripten"',
					{
						'defines':
						[
							'TARGET_SUPPORTS_JAVA',
						],
						'include_dirs':
						[
							'<(javahome)/include',
							'<(javahome)/include/darwin',
						],
						'sources':
						[
							'<(INTERMEDIATE_DIR)/src/libfoundationjvm.stubs.cpp',
						],
					},
				],
				[
					'host_os == "linux" and OS != "emscripten"',
					{
						'defines':
						[
							'TARGET_SUPPORTS_JAVA',
						],
						'include_dirs':
						[
							'<(javahome)/include',
							'<(javahome)/include/linux',
						],
						'sources':
						[
							'<(INTERMEDIATE_DIR)/src/libfoundationjvm.stubs.cpp',
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
