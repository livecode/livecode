{
	'variables':
	{
		'target_sdk%': 'macosx10.8',
		'host_sdk%': 'macosx',
		
		'output_dir': '../mac-bin',
	},

	'xcode_config_file': '../version',
	
	'xcode_settings':
	{
		'SDKROOT': '<(target_sdk)',

		'SOLUTION_DIR': '<(DEPTH)',
		'SYMROOT': '$(SOLUTION_DIR)/_build/mac',
		'OBJROOT': '$(SOLUTION_DIR)/_cache/mac',
		'CONFIGURATION_BUILD_DIR': '$(SYMROOT)/$(CONFIGURATION)',
		'CONFIGURATION_TEMP_DIR': '$(OBJROOT)/$(CONFIGURATION)',
		'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
		'GCC_ENABLE_CPP_RTTI': 'NO',
		'GCC_THREADSAFE_STATICS': 'NO',
		'SHARED_PRECOMPS_DIR': '$(OBJROOT)/Precompiled/$(CURRENT_ARCH)',
		'GCC_WARN_ABOUT_DEPRECATED_FUNCTIONS': 'NO',
		'ALWAYS_SEARCH_USER_PATHS': 'NO',
		'MACOSX_DEPLOYMENT_TARGET': '10.6',
	},
	
	'target_defaults':
	{
		'mac_bundle': 0,
		
		'variables':
		{
			'app_bundle_suffix': '.app',
			'ext_bundle_suffix': '.bundle',
			'exe_suffix': '',
			'lib_suffix': '.dylib',
			'ext_suffix': '.so',
		},
		
		'target_conditions':
		[
			[
				'server_mode == 0',
				{
					'defines':
					[
						'TARGET_PLATFORM_MACOS_X',
						'_MACOSX',
					],
				},
				{
					'defines':
					[
						'_MACOSX',

						'_SERVER',
						'_MAC_SERVER',
					],
				},
			],
			[
				# Non-bundle loadable module should have a .dylib suffix
				'_type == "loadable_module" and _mac_bundle == 0',
				{
					'product_extension': 'dylib',
				},
			],
		],
		
		'configurations':
		{
			'Debug':
			{
				'defines':
				[
					'_DEBUG',
				],
			},
			
			'Release':
			{
				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},
			
			'Fast':
			{
				'defines':
				[
					'_RELEASE',
					'NDEBUG',
				],
			},
		},
	},
	
	'configurations':
	{
		'Debug':
		{
			'xcode_settings':
			{
				'ARCHS': 'i386',
				'COPY_PHASE_STRIP': 'NO',
				'GCC_OPTIMIZATION_LEVEL': '0',
			},
		},
		
		'Release':
		{
			'xcode_settings':
			{
				'ARCHS': 'i386',
				'GCC_OPTIMIZATION_LEVEL': '3',
				'GCC_ENABLE_FIX_AND_CONTINUE': 'NO',
			},
		},
		
		'Fast':
		{
			'xcode_settings':
			{
				'ARCHS': 'i386',
				'GCC_OPTIMIZATION_LEVEL': '0',
				'GCC_ENABLE_FIX_AND_CONTINUE': 'NO',
			},
		},
	},
}
