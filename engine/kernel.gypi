{
	'targets':
	[
		{
			'target_name': 'kernel',
			'type': 'static_library',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				#'../libexternal/libexternal.gyp:libExternal',
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
				'<@(engine_desktop_source_files)',
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
					'OS == "android"',
					{
						'dependencies':
						[
							'../thirdparty/libfreetype/libfreetype.gyp:libfreetype',
							'../thirdparty/libskia/libskia.gyp:libskia',
						],
						
						'sources!':
						[
							# Not yet supported on Android
							'src/mblactivityindicator.cpp',
							'src/mblcamera.cpp',
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
								'$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
								'$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
								'$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/Security.framework',
								'$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
							],
						},
					],
					[
						'OS == "mac"',
						{
							'libraries':
							[
								'$(SDKROOT)/usr/lib/libcups.dylib',
								'$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
								'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
								'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
								'$(SDKROOT)/System/Library/Frameworks/Quartz.framework',
							],
						},
					],
					[
						'OS == "ios"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/AddressBook.framework',
								'$(SDKROOT)/System/Library/Frameworks/AddressBookUI.framework',
								'$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CFNetwork.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreLocation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreMotion.framework',
								'$(SDKROOT)/System/Library/Frameworks/EventKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/EventKitUI.framework',
								'$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/MediaPlayer.framework',
								'$(SDKROOT)/System/Library/Frameworks/MessageUI.framework',
								'$(SDKROOT)/System/Library/Frameworks/OpenGLES.framework',
								'$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
								'$(SDKROOT)/System/Library/Frameworks/StoreKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/UIKit.framework',
							],	
						},
					],
					[
						'OS == "linux"',
						{
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
							
							'actions':
							[
								{
									'action_name': 'linux_library_stubs',
									'inputs':
									[
										'../util/weak_stub_maker.pl',
										'src/linux.stubs',
									],
									'outputs':
									[
										'<(SHARED_INTERMEDIATE_DIR)/src/linux.stubs.cpp',
									],
									
									'action':
									[
										'<@(perl)',
										'../util/weak_stub_maker.pl',
										'src/linux.stubs',
										'<@(_outputs)',
									],
								},
							],
						},
					],
					[
						'OS == "android"',
						{
							'libraries':
							[
								'-lGLESv1_CM',
								'-ljnigraphics',
								'-llog',
								'-lm',
								'-lstdc++',
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
		
		{
			'target_name': 'kernel-java',
			'type': 'none',
			
			# A little indirection needed to get INTERMEDIATE_DIR escaped properly
			'intermediate_dir_escaped': '<!(["sh", "-c", "echo $1 | sed -e $2", "echo", "<(INTERMEDIATE_DIR)", "s/\\\\$/\\\\\\$/g"])',
			
			'sources':
			[
				'<@(engine_aidl_source_files)',
				
				# Outputs from a rule don't get considered as
				# inputs to another rule in Gyp, unfortunately
				'<!@((for x in <@(engine_aidl_source_files); do echo "<(_intermediate_dir_escaped)/${x}"; done) | sed -e "s/\\.aidl$/\\.java/g")',
				
				# Some of the Java sources depend on the output
				# from AIDL so they come last
				'<@(engine_java_source_files)',
			],
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'rules':
						[
								
							{
								'rule_name': 'aidl_interface_gen',
								'extension': 'aidl',
					
								'message': '  AIDL <(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).java',
								'process_outputs_as_sources': 1,
					
								'inputs':
								[
									'<(aidl_framework_path)',
								],
					
								'outputs':
								[
									'<(INTERMEDIATE_DIR)/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).java',
								],
					
								'action':
								[
									'<(aidl_path)',
									'-Isrc/java',
									'-p' '<@(_inputs)',
									'-o' '<(INTERMEDIATE_DIR)/src/java',
									'<(RULE_INPUT_PATH)',
								],
							},
				
							{
								'rule_name': 'javac',
								'extension': 'java',
					
								'message': '  JAVAC <(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).class',
					
								'outputs':
								[
									# Java writes the output file based on the class name.
									# Use some Make nastiness to correct the output name
									'<(PRODUCT_DIR)/classes_livecode_community/$(subst\t<(INTERMEDIATE_DIR)/,,$(subst\tsrc/java/,,<(RULE_INPUT_DIRNAME)))/<(RULE_INPUT_ROOT).class',
								],
					
								'action':
								[
									'<(javac_path)',
									'-d', '<(PRODUCT_DIR)/classes_livecode_community',
									'-source', '1.5',
									#'-target', '1.5',
									'-implicit:none',
									'-cp', '<(java_classpath)',
									'-sourcepath', 'src/java:<(INTERMEDIATE_DIR)/src/java',
									'<(RULE_INPUT_PATH)',
								],
							},
						],
					}
				]
			],
		},
	],
}
