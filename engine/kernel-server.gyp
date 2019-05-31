{
	'variables':
	{
		'module_name': 'kernel-server',
		'module_test_dependencies':
		[
			'kernel-server',
			'../libfoundation/libfoundation.gyp:libFoundation',
			'../libgraphics/libgraphics.gyp:libGraphics',
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
			
			'toolsets': ['host', 'target'],

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
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../libgraphics/libgraphics.gyp:libGraphics',
				'../libscript/libscript.gyp:libScript',
				'../libscript/libscript.gyp:stdscript',
				
				'../prebuilt/libcurl.gyp:libcurl',
				'../prebuilt/libopenssl.gyp:libopenssl',

				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_pcre',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_jpeg',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_gif',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_png',
		
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_z',
		
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
						'dependencies':
						[
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_cairo',
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
								'$(SDKROOT)/usr/lib/libcups.dylib',
								'$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
								'$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
								'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
								'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
								'$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/Security.framework',
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
								'-Wl,-Bstatic',
								'-lstdc++',
								'-Wl,-Bdynamic',
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
