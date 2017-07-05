{
	'variables':
	{
		'module_name': 'kernel-server',
		'module_test_dependencies':
		[
			'kernel-server',
			'../libfoundation/libfoundation.gyp:libFoundation',
			'../libgraphics/libgraphics.gyp:libGraphics',
			'libplatform.gyp:libplatform',
		],
		'module_test_sources':
		[
			'<@(engine_test_source_files)',
			'<@(engine_security_source_files)',
		],
		'module_test_include_dirs':
		[
			'include',
			'src',
		],
		'module_test_defines': [ 'MODE_SERVER', ],
	},

	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
		'../config/cpptest.gypi'
	],

	'targets':
	[
		{
			'target_name': 'kernel-server',
			'type': 'static_library',
			
			'includes':
			[
				'kernel-mode-template.gypi',
			],
			
			'variables':
			{
				'server_mode': 1,
				'mode_macro': 'MODE_SERVER',
			},
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				
				'../prebuilt/libcurl.gyp:libcurl',
				'../prebuilt/libopenssl.gyp:libopenssl',
				
				'../thirdparty/libgif/libgif.gyp:libgif',
				'../thirdparty/libjpeg/libjpeg.gyp:libjpeg',
				'../thirdparty/libpcre/libpcre.gyp:libpcre',
				'../thirdparty/libpng/libpng.gyp:libpng',
				'../thirdparty/libz/libz.gyp:libz',
				
				'engine-common.gyp:quicktime_stubs',
				
				'lcb-modules.gyp:engine_lcb_modules',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'<@(engine_common_source_files)',
				'<@(engine_server_source_files)',
				'<@(engine_module_source_files)',
			],
			
			'sources!':
			[
				'<@(engine_server_exclude_files)',
			],
			
			'conditions':
			[
				[
					'OS == "linux"',
					{
						'include_dirs':
						[
							'../thirdparty/headers/linux/include/cairo',
						],
						
						'defines':
                        [
	                        'PANGO_ENABLE_BACKEND',
	                        'PANGO_ENABLE_ENGINE',
	                    ],
					},
				],
				[
					'mobile != 0',
					{
						'type': 'none',
					},
				],
				[
					'OS == "mac"',
					{
						'defines':
						[
							# We want to use the new prototypes for the Objective-C
							# dispatch methods as it helps catch certain errors at
							# compile time rather than run time.
							'OBJC_OLD_DISPATCH_PROTOTYPES=0',
						],
					},
				],
			],
			
			'link_settings':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
								'$(SDKROOT)/usr/lib/libcups.dylib',
								'$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
								'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
								'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
								'$(SDKROOT)/System/Library/Frameworks/Quartz.framework',
								'$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
								'$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/Security.framework',
								'$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
								'$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
							],
						},
					],
					[
						'OS == "linux"',
						{
							'dependencies':
							[
								#'engine.gyp:create_linux_stubs',
							],
							
							'sources':
							[
								'<(SHARED_INTERMEDIATE_DIR)/src/linux.stubs.cpp',
							],
							
							'libraries':
							[
								'-ldl',
								'-lpthread',
							],
						},
					],
					[
						'OS == "win"',
						{
							'libraries':
							[
								'-ladvapi32',
								'-lcomdlg32',
								'-lcrypt32',
								'-lgdi32',
								'-liphlpapi',
								'-limm32',
								'-lmscms',
								'-lmsimg32',
								'-lpsapi',
								'-lole32',
								'-loleaut32',
								'-lrpcrt4',
								'-lshell32',
								'-lshlwapi',
								'-luser32',
								'-lusp10',
								'-lversion',
								'-lwinmm',
								'-lwinspool',
								'-lws2_32',
							],
						},
					],
				],
			},
		},
	],
}
