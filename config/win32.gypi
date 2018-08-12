{
	'variables':
	{
		# Path to versions 4 and 5 of the Microsoft Speech SDK
		'ms_speech_sdk4%': '$(foo)C:/Program Files/Microsoft Speech SDK',
		'ms_speech_sdk5%': '$(foo)C:/Program Files/Microsoft Speech SDK 5.1',
		
		# Set if the Gyp step is being run on a Unix-like host (i.e not Windows)
		'unix_configure%': '0',
		
		'output_dir': '../win-<(uniform_arch)-bin',
	},
	
	'target_defaults':
	{
		'variables':
		{
			'app_bundle_suffix': '.exe',
			'ext_bundle_suffix': '.dll',
			'lib_suffix': '.dll',
			'ext_suffix': '.dll',
			'exe_suffix': '.exe',
			'debug_info_suffix': '',

			'silence_warnings': 0,
			'msvs_compiler_version': "141",
		},
		
		# Don't assume a Cygwin environment when invoking actions
		'msvs_cygwin_shell': 0,
		
		# TODO [2017-04-11]: Remove these overrides when we can use 	
		# -Gmsvs_version=2017
		"msvs_target_platform_version" : "10.0.14393.0",
		"msbuild_toolset" : "v<(msvs_compiler_version)",
		
		# WIN64-CHECK
		'conditions':
		[
			[
				'target_arch == "x64"',
				{
					'msvs_target_platform': 'x64',
					'msvs_configuration_platform': 'x64',
				},
				
			],
		],

		'configurations':
		{
			'Debug':
			{	
				'variables':
				{
					'msvs_crt_mode': 'mtd',
				},
				
				'msvs_settings':
				{
					'VCCLCompilerTool':
					{
						'Optimization': '0',
						'PreprocessorDefinitions': ['_DEBUG'],
						'RuntimeLibrary': '1',
						'DebugInformationFormat': '4',
					},
					
					'VCLinkerTool':
					{
						'OptimizeReferences': '2',
						'GenerateDebugInformation': 'true',
						'EnableCOMDATFolding': '2',
					},
				},
				
				'target_conditions':
				[
					[
						'silence_warnings == 0',
						{
							'msvs_settings':
							{
								'VCCLCompilerTool':
								{
									'BasicRuntimeChecks': 3,
								},
							},
						},
					],
				],
			},
			
			'Release':
			{
				'variables':
				{
					'msvs_crt_mode': 'mt',
				},
				
				'msvs_settings':
				{
					'VCCLCompilerTool':
					{
						'Optimization': '3',
						'WholeProgramOptimization': 'false',
						'PreprocessorDefinitions': [ '_RELEASE', 'NDEBUG' ],
						'RuntimeLibrary': '0',
						'DebugInformationFormat': '3',
					},
					
					'VCLinkerTool':
					{
						'LinkIncremental': '1',
						'OptimizeReferences': '2',
						'GenerateDebugInformation': 'true',
						'EnableCOMDATFolding': '2',
					},
				},
			},
			
			'Fast':
			{
			
				'variables':
				{
					'msvs_crt_mode': 'mt',
				},
			
				'msvs_settings':
				{
					'VCCLCompilerTool':
					{
						'Optimization': '0',
						'WholeProgramOptimization': 'false',
						'PreprocessorDefinitions': ['_RELEASE', 'NDEBUG'],
						'RuntimeLibrary': '0',
						'DebugInformationFormat': '0',
					},
					
					'VCLinkerTool':
					{
						'LinkIncremental': '0',
						'GenerateDebugInformation': 'false',
						'OptimizeReferences': '0',
						'EnableCOMDATFolding': '0',
					},
				},
			},
		},
		
		'defines':
		[
			'_CRT_NONSTDC_NO_DEPRECATE',
			'_CRT_SECURE_NO_DEPRECATE',
			'_CRT_DISABLE_PERFCRIT_LOCKS',
			'__LITTLE_ENDIAN__',
			'WINVER=0x0601',        # Windows 7
			'_WIN32_WINNT=0x0601',  # Windows 7
		],
		
		'target_conditions':
		[
			[
				'server_mode == 0',
				{
					'defines':
					[
						'_WINDOWS',
						'WIN32',
						'TARGET_PLATFORM_WINDOWS',
					],
				},
				{
					'defines':
					[
						'_SERVER',
						'_WINDOWS_SERVER',
						'WIN32',
					],
				},
			],
			[
				'silence_warnings == 0',
				{
					'msvs_settings':
					{
						'VCCLCompilerTool':
						{
							'WarningLevel': '3',
						},
					},
				},
				{
					'msvs_settings':
					{
						'VCCLCompilerTool':
						{
							'WarningLevel': '0',
						},

						'MASM':
						{
							'WarningLevel': '0',
						}
					},
				},
			],
		],
		
		'msvs_settings':
		{
			'VCCLCompilerTool':
			{
				'ExceptionHandling': '0',
				'BufferSecurityCheck': 'false',
				'RuntimeTypeInfo': 'false',
				'Detect64BitPortabilityProblems': 'true',

				# Silence abundent warnings to speed up build:
				#   4577: exception handling mode mismatch
				#   4800: performance warning about cast to bool
				#   4244: possible loss of data due to int-like truncation
				'DisableSpecificWarnings': '4577;4800;4244',
			},
			
			'VCLibrarianTool':
			{
				'AdditionalOptions':
				[
					'/MACHINE:<(target_arch)',
				],
			},

			'VCLinkerTool':
			{
				'SubSystem': '2',
				'RandomizedBaseAddress': '1',	# /DYNAMICBASE:NO - disable ASLR
				'ImageHasSafeExceptionHandlers': 'false',
				'AdditionalOptions':
				[
					'/MACHINE:<(target_arch)',
				],
			},
		},
	},
}
