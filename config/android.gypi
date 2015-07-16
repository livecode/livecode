{
	'variables':
	{
		'mobile': 1,
		
		'variables':
		{
			'java_sdk_path%': '<!(echo ${JAVA_SDK})',
			'android_sdk_path%': '<!(echo ${ANDROID_SDK})',
			'android_ndk_path%': '<!(echo ${ANDROID_NDK})',
			'android_platform%': '<!(echo ${ANDROID_PLATFORM})',
			'android_build_tools%': '<!(echo ${ANDROID_BUILD_TOOLS})',
		},
		
		'javac_path': '<(java_sdk_path)/bin/javac',
		'jar_path': '<(java_sdk_path)/bin/jar',
		
		'aidl_path': '<(android_sdk_path)/build-tools/<(android_build_tools)/aidl',
		
		'aidl_framework_path': '<(android_sdk_path)/platforms/<(android_platform)/framework.aidl',
		
		'java_classpath': '<(android_sdk_path)/platforms/<(android_platform)/android.jar',
		
		'output_dir': '../android-<(target_arch)-bin',
		
		# Capture the values of some build tool environment vars
		'objcopy': '<!(echo ${OBJCOPY:-objcopy})',
		'objdump': '<!(echo ${OBJDUMP:-objdump})',
		'strip':   '<!(echo ${STRIP:-strip})',
	},
	
	'target_defaults':
	{
		'variables':
		{
			'app_bundle_suffix': '',
			'ext_bundle_suffix': '',
			'lib_suffix': '',
			'ext_suffix': '',
			'exe_suffix': '',
			'debug_info_suffix': '.dbg',
			
			'silence_warnings': 0,
		},
		
		'cflags':
		[
			'-fstrict-aliasing',
			'-fvisibility=hidden',
		],
		
		'cflags_c':
		[
			'-std=gnu99',
			'-Wstrict-prototypes',
		],
		
		'cflags_cc':
		[
			'-std=c++03',
			'-fno-exceptions',
			'-fno-rtti',
		],
		
		'target_conditions':
		[
			[
				'silence_warnings == 0',
				{
					'cflags':
					[
						'-Wall',
						'-Wextra',
						'-Wno-unused-parameter',	# Just contributes build noise
					],
					
					'cflags_c':
					[
						'-Werror=declaration-after-statement',	# Ensure compliance with C89
					],
				},
				{
					'cflags':
					[
						'-w',						# Disable warnings
						'-fpermissive',				# Be more lax with old code
					],
				},
			],
		],
		
		'configurations':
		{
			'Debug':
			{
				'cflags':
				[
					'-O0',
					'-g3',
				],
				
				'defines':
				[
					'_DEBUG',
				],
			},
			
			'Release':
			{
				'cflags':
				[
					'-O3',
					'-g3',
				],
				
				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},
			
			'Fast':
			{
				'cflags':
				[
					'-O0',
					'-g0',
				],
				
				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},
		},
		
		'defines':
		[
			'TARGET_PLATFORM_MOBILE',
			'TARGET_SUBPLATFORM_ANDROID',
			'ANDROID',
			'_MOBILE',
			'ANDROID_NDK',
		],
	},
}
