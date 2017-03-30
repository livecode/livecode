{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libplatform',
			'type': 'static_library',
			
			'include_dirs':
			[
				'include',
				'src',
				'../libgraphics/include',
				'../libscript/include',
				'../libfoundation/include',
			],
			
			'sources':
			[
				'<@(engine_platform_source_files)',
			],
			
			'conditions':
			[
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
                        # 'type': 'shared_library',
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
						'OS == "mac" and target_sdk != "macosx10.6"',
                        {
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
                        'OS == "mac" and target_sdk == "macosx10.6"',
                        {
                            'libraries!':
                            [
                                '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework',
                                '$(SDKROOT)/System/Library/Frameworks/CoreMedia.framework',
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
								'-lstrmiids',
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
