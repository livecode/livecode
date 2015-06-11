{
	'variables':
	{
		'mobile': 1,
		'target_sdk%': 'iphoneos8.3',
		'host_sdk%': 'macosx',
		
		'output_dir': '../ios-bin/<(target_sdk)',
		
		'mac_tools_dir': '$(SOLUTION_DIR)/_build/ios/<(host_sdk)/$(CONFIGURATION)'
	},
	
	'xcode_config_file': '../version',
	
	'xcode_settings':
	{
		'SDKROOT': '<(target_sdk)',
		'SUPPORTED_PLATFORMS': 'iphoneos iphonesimulator',

		'SOLUTION_DIR': '<(DEPTH)',
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
			
			'mac_tools_dir': '$(SOLUTION_DIR)/_build/ios/<(host_sdk)/$(CONFIGURATION)'
		},
		
		'mac_bundle': 0,
		
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
				# If building a host tool, use the OSX settings
				'_toolset != "target"',
				{
					'xcode_settings':
					{
						'SDKROOT[platform=macosx*]': '<(host_sdk)',
						'SUPPORTED_PLATFORMS': 'macosx',
						'ARCHS': 'x86_64',
						
						'SYMROOT': '$(SOLUTION_DIR)/_build/ios/<(host_sdk)',
						'OBJROOT': '$(SOLUTION_DIR)/_cache/ios/<(host_sdk)',
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
