{
	'variables':
	{
		'mobile': 1,
		
		'variables':
		{
			'java_sdk_path%': '<!(echo ${JAVA_SDK})',
			'android_sdk_path%': '<!(echo ${ANDROID_SDK})',
			'android_ndk_path%': '<!(echo ${ANDROID_NDK})',
			'android_api_version%': '<!(echo ${ANDROID_API_VERSION})',
			'android_ndk_platform_version%': '<!(echo ${ANDROID_NDK_PLATFORM_VERSION})',
			'android_ndk_version%': '<!(echo ${ANDROID_NDK_VERSION})',
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

		'android_ndk_path%': '<(android_ndk_path)',
		'android_subplatform%': 'ndk<(android_ndk_platform_version)<(android_ndk_version)',

		'android_play_path': '<(android_sdk_path)/extras/google/m2repository/com/google/android/gms',
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
	
		'target_conditions':
		[
			[
				'_toolset == "host"',
				{
					'includes':
					[
						'linux-settings.gypi',
					],
				},
				{
					'includes':
					[
						'android-settings.gypi',
					],
				},
			],
		],
	},
}
