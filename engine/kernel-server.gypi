{
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
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../libexternal/libexternal.gyp:libExternal',
				'../libgraphics/libgraphics.gyp:libGraphics',
				
				'../prebuilt/libcurl.gyp:libcurl',
				'../prebuilt/libopenssl.gyp:libopenssl',
				
				'../thirdparty/libgif/libgif.gyp:libgif',
				'../thirdparty/libjpeg/libjpeg.gyp:libjpeg',
				'../thirdparty/libpcre/libpcre.gyp:libpcre',
				'../thirdparty/libpng/libpng.gyp:libpng',
				'../thirdparty/libz/libz.gyp:libz',
				
				'encode_version',
				'quicktime_stubs',
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
					},
				],
				[
					'OS == "win"',
					{
						'include_dirs':
						[
							'<(quicktime_sdk)/CIncludes',
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
						'OS == "mac" or OS == "ios"',
						{
							'libraries':
							[
								'$(SDKROOT)/usr/lib/libcups.dylib',
								'$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
								'$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
								'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
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
								'-lX11',
								'-lXext',
							],
						},
					],
					[
						'OS == "win"',
						{
							'library_dirs':
							[
								'<(quicktime_sdk)/Libraries',
							],
							
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
								'-lwinmm',
								'-lwinspool',
								'-lws2_32',
								
								'-lQTMLClient',
								'-lQTVR',
							],
						},
					],
				],
			},
		},
	],
}
