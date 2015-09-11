{
	'variables':
	{
		'mobile': 1,
		'target_sdk%': 'iphoneos8.3',
		'host_sdk%': 'macosx',
		
		'output_dir': '../ios-bin/<(target_sdk)',
	},
	
	'xcode_config_file': '../version',
	
	'xcode_settings':
	{
		'SDKROOT': '<(target_sdk)',

		'SOLUTION_DIR': '$(SOURCE_ROOT)/<(DEPTH)',
		'SYMROOT': '$(SOLUTION_DIR)/_build/ios/$(SDK_NAME)',
		'OBJROOT': '$(SOLUTION_DIR)/_cache/ios/$(SDK_NAME)',
		'CONFIGURATION_BUILD_DIR': '$(SYMROOT)/$(CONFIGURATION)',
		'CONFIGURATION_TEMP_DIR': '$(OBJROOT)/$(CONFIGURATION)',
		'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',
		'GCC_ENABLE_CPP_RTTI': 'NO',
		'GCC_THREADSAFE_STATICS': 'NO',
		'SHARED_PRECOMPS_DIR': '$(OBJROOT)/Precompiled/$(CURRENT_ARCH)',
		'GCC_VERSION': '',
		'ALWAYS_SEARCH_USER_PATHS': 'NO',
		'IPHONEOS_DEPLOYMENT_TARGET[arch=armv7]': '5.1.1',
		'IPHONEOS_DEPLOYMENT_TARGET[arch=i386]': '5.1.1',
		'IPHONEOS_DEPLOYMENT_TARGET[arch=arm64]': '7.0.0',
		'IPHONEOS_DEPLOYMENT_TARGET[arch=x86_64]': '7.0.0',
		'ENABLE_BITCODE[arch=armv7]': 'No',
		'ENABLE_BITCODE[arch=i386]': 'No',
		'ENABLE_BITCODE[arch=arm64]': 'Yes',
		'ENABLE_BITCODE[arch=x86_64]': 'Yes',
		'TARGETED_DEVICE_FAMILY': '1,2',
		'DEBUG_INFORMATION_FORMAT': 'dwarf-with-dsym',
		'ARCHS_STANDARD': 'armv7 arm64',
		'CLANG_CXX_LIBRARY': 'libc++',
		'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES',
		'COPY_PHASE_STRIP': 'NO',
		'STRIP_INSTALLED_PRODUCT': 'NO',
		
		'CODE_SIGN_IDENTITY[sdk=iphoneos*]': 'iPhone Developer',
	},
	
	'target_defaults':
	{
		'variables':
		{
			'ios_external_symbol_list': '',
			
			'app_bundle_suffix': '.ios-engine',
			'ext_bundle_suffix': '.ios-extension',
			'exe_suffix': '',
			'lib_suffix': '.dylib',
			'ext_suffix': '.so',
			'debug_info_suffix': '.dSYM',
			
			'silence_warnings': 0,
		},
		
		'defines':
		[
			'_MOBILE',
			'TARGET_PLATFORM_MOBILE',
			'TARGET_SUBPLATFORM_IPHONE',
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
		
		'target_conditions':
		[
			[
				# Convert externals into the correct form for iOS
				'_type == "loadable_module"',
				{
					'type': 'static_library',
					'mac_bundle': 0,

					'xcode_settings':
					{
						'MACH_O_TYPE': 'staticlib',
					},

					'postbuilds':
					[
						{
							'postbuild_name': 'create-ios-external',
							'message': 'Creating ">(_target_name)" iOS external',
							
							'inputs':
							[
								'../tools/build-extension-ios.sh',
							],
							
							'action':
							[
								'<@(_inputs)',
								'>(ios_external_symbols)',
								'>(ios_external_symbol_list)',
							],
						},
					],
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
	},
	
	'configurations':
	{
		'Debug':
		{
			'xcode_settings':
			{
				'GCC_OPTIMIZATION_LEVEL': '0',
				'ARCHS': '<(target_arch)'
			},
		},
		
		'Release':
		{
			'xcode_settings':
			{
				'GCC_OPTIMIZATION_LEVEL': '3',
				'GCC_THUMB_SUPPORT[arch=armv6]': 'NO',
				'GCC_THUMB_SUPPORT[arch=armv7]': 'YES',
				'ARCHS': '<(target_arch)'
			},
		},
		
		'Fast':
		{
			'xcode_settings':
			{
				'GCC_OPTIMIZATION_LEVEL': '0',
				'ARCHS': '<(target_arch)',
			},
		},
	},
}
