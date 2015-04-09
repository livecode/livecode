{
	'variables':
	{
		'mobile': 1,
		
		'variables':
		{
			'java_sdk_path%': '<!(echo ${JAVA_SDK:-/usr/lib/jvm/java-6-openjdk-amd64})',
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
	},
	
	'target_defaults':
	{
		'cflags_cc':
		[
			'-fno-exceptions',
			'-fno-rtti',
		],
		
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
