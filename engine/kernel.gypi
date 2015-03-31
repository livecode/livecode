{
	'targets':
	[
		{
			'target_name': 'kernel',
			'type': 'static_library',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
				'../libgraphics/libgraphics.gyp:libGraphics',
				
				'../thirdparty/libgif/libgif.gyp:libgif',
				'../thirdparty/libjpeg/libjpeg.gyp:libjpeg',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl',
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
							],
						},
					],
					[
						'OS == "linux"',
						{
							'sources':
							[
								'<(INTERMEDIATE_DIR)/src/linux.stubs.cpp',
							],
							
							'libraries':
							[
								'-ldl',
								'-lpthread',
								'-lX11',
								'-lXext',
							],
							
							'actions':
							[
								{
									'action_name': 'linux_library_stubs',
									'inputs':
									[
										'../tools/weak_stub_maker.lc',
										'src/linux.stubs',
									],
									'outputs':
									[
										'<(INTERMEDIATE_DIR)/src/linux.stubs.cpp',
									],
									
									'action':
									[
										'<(revolution_path)',
										'../tools/weak_stub_maker.lc',
										'src/linux.stubs',
										'<@(_outputs)',
									],
								},
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
