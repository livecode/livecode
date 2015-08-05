{
	'targets':
	[
		{
			'target_name': 'kernel',
			'type': 'static_library',
			
			'dependencies':
			[
				'../libfoundation/libfoundation.gyp:libFoundation',
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
				'<@(engine_java_source_files)',
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
										
						# Force the entry point to be included in the output
						'link_settings':
						{
							'ldflags':
							[
								'-Wl,--undefined,Java_com_runrev_android_Engine_doCreate'
							],
						},
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
								'$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
								'$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
								'$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
								'$(SDKROOT)/System/Library/Frameworks/Security.framework',
								'$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
							],
							
							# Adding AVFoundation in the list of libraries does not allow
							# us to weak link it. Only adding the linking flag does the job
							'xcode_settings':
							{
								'OTHER_LDFLAGS':
								[
									'-weak_framework AVFoundation',
								]
							},
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
								'$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
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
						'OS == "android"',
						{
							'libraries':
							[
								'-lGLESv1_CM',
								'-ljnigraphics',
								'-llog',
								'-lm',
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
		
		{
			'target_name': 'kernel-java',
			'type': 'none',
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'variables':
						{
							'java_classes_dir_name': 'classes_livecode_community',
						},
						
						# Include the rules for compiling Java
						'includes':
						[
							'../config/java.gypi',
						],
						
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
					}
				]
			],
		},
	],
}
