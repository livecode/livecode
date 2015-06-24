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
		'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
		'COPY_PHASE_STRIP': 'NO',
		'STRIP_INSTALLED_PRODUCT': 'NO',
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
			'debug_info_suffix': '.dSYM',
			
			'silence_warnings': 0,
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
				# and be linked as libraries, not bundles
				'_type == "loadable_module" and _mac_bundle == 0',
				{
					'product_extension': 'dylib',
					'xcode_settings':
					{
						'MACH_O_TYPE': 'mh_dylib',
					},
				},
			],
			[
				'silence_warnings == 0',
				{
					'xcode_settings':
					{
						'GCC_WARN_CHECK_SWITCH_STATEMENTS': 'YES',
						'CLANG_WARN_EMPTY_BODY': 'YES',
						'GCC_WARN_SHADOW': 'YES',
						'CLANG_WARN_BOOL_CONVERSION': 'YES',
						'CLANG_WARN_CONSTANT_CONVERSION': 'YES',
						'GCC_WARN_64_TO_32_BIT_CONVERSION': 'YES',
						'CLANG_WARN_ENUM_CONVERSION': 'YES',
						'CLANG_WARN_INT_CONVERSION': 'YES',
						'CLANG_WARN_IMPLICIT_SIGN_CONVERSION': 'YES',
						'GCC_WARN_INITIALIZER_NOT_FULLY_BRACKETED': 'YES',
						'GCC_WARN_ABOUT_RETURN_TYPE': 'YES',
						'GCC_WARN_MISSING_PARENTHESES': 'YES',
						'GCC_WARN_ABOUT_MISSING_FIELD_INITIALIZERS': 'YES',
						'GCC_WARN_ABOUT_MISSING_PROTOTYPES': 'NO',
						'GCC_WARN_ABOUT_MISSING_NEWLINE': 'YES',
						'CLANG_WARN_ASSIGN_ENUM': 'YES',
						'GCC_WARN_ABOUT_POINTER_SIGNEDNESS': 'YES',
						'GCC_WARN_SIGN_COMPARE': 'YES',
						'CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION': 'YES',
						'GCC_WARN_TYPECHECK_CALLS_TO_PRINTF': 'YES',
						'GCC_WARN_UNINITIALIZED_AUTOS': 'YES',
						'GCC_WARN_UNKNOWN_PRAGMAS': 'YES',
						'CLANG_WARN_UNREACHABLE_CODE': 'YES',
						'GCC_WARN_UNUSED_FUNCTION': 'YES',
						'GCC_WARN_UNUSED_LABEL': 'YES',
						'GCC_WARN_UNUSED_PARAMETER': 'NO',
						'GCC_WARN_UNUSED_VALUE': 'YES',
						'GCC_WARN_UNUSED_VARIABLE': 'YES',

						'WARNING_CFLAGS': 
						[ 
							'-Wall', 
							'-Wextra', 
							'-Werror=declaration-after-statement',
							'-Wno-unused-parameter',
						],
					},
				},
				{
					'xcode_settings':
					{
						'GCC_INHIBIT_ALL_WARNINGS': 'YES',
					},
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
