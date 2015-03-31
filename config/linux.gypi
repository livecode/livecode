{
	'target_defaults':
	{
		'defines':
		[
			'_LINUX',
			'LINUX',
			'X11',
			'GTKTHEME',
			'HAVE___THREAD',
			'TARGET_PLATFORM_LINUX',
			'TARGET_PLATFORM_POSIX',
			'_FILE_OFFSET_BITS=64',			
		],
		
		# We supply some pre-packaged headers for Linux libraries
		'include_dirs':
		[
			'../thirdparty/headers/linux/include',
		],
	},
}
