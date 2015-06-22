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
			'debug_info_suffix': '.dbg',
			
			'c++_std': '<!(echo ${CXX_STD:-gnu++03})',
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
			'../thirdparty/libcairo/src',			# Required by the GDK headers
			'../thirdparty/libfreetype/include',	# Required by the Pango headers
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
				'_type == "loadable_module" or _type == "shared_library" or (_type == "static_library" and library_for_module != 0)',
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
						'_LINUX',
						'_SERVER',
						'_LINUX_SERVER',
					],
				},
			],
		],
		
		'cflags':
		[
			'-Wall',
			'-Wextra',
			'-fstrict-aliasing',
		],
		
		'cflags_c':
		[
			'-std=gnu99',
			'-Wstrict-prototypes',
		],
		
		'cflags_cc':
		[
			'-std=<(c++_std)',
			'-fno-exceptions',
			'-fno-rtti',
		],
		
		'configurations':
		{
			'Debug':
			{
				'cflags':
				[
					'-Og',
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
	},
}
