{
	'variables':
	{
		'output_dir': '../linux-<(target_arch)-bin',
	},
	
	'target_defaults':
	{
		'variables':
		{
			'app_bundle_suffix': '',
			'ext_bundle_suffix': '.so',
			'lib_suffix': '.so',
			'ext_suffix': '.so',
			'exe_suffix': '',
		},
		
		'defines':
		[
			'HAVE___THREAD',
			'_FILE_OFFSET_BITS=64',			
		],
		
		# We supply some pre-packaged headers for Linux libraries
		'include_dirs':
		[
			'../thirdparty/headers/linux/include',
		],
		
		# Disable exceptions and RTTI, except where needed
		'cflags_cc':
		[
			'-fno-exceptions',
			'-fno-rtti',
		],
		
		# Static libraries that are to be included into dynamic libraries
		# need to be compiled with the correct compilation flags
		'target_conditions':
		[
			[
				'_type == "loadable_module" or (_type == "static_library" and library_for_module != 0)',
				{
					'cflags':
					[
						'-fPIC',
					],
				},
			],
			[
				'server_mode == 0',
				{
					'defines':
					[
						'TARGET_PLATFORM_LINUX',
						'TARGET_PLATFORM_POSIX',
						'GTKTHEME',
						'LINUX',
						'_LINUX',
						'X11',
					],
				},
				{
					'defines':
					[
						'_SERVER',
						'_LINUX_SERVER',
					],
				},
			],
		],
	},
}
