{
	'xcode_config_file': '../version',
	
	'xcode_settings':
	{
		'SDKROOT': 'macosx10.8',
		
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
						'_SERVER',
						'_MAC_SERVER',
					],
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
	},
}
