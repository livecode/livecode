{
	'variables':
	{
		# Path to the Windows SDK for Apple QuickTime
		'quicktime_sdk%': '$(foo)C:/Program Files/QuickTime SDK',
		
		# Path to versions 4 and 5 of the Microsoft Speech SDK
		'ms_speech_sdk4%': '$(foo)C:/Program Files/Microsoft Speech SDK',
		'ms_speech_sdk5%': '$(foo)C:/Program Files/Microsoft Speech SDK 5.1',
		
		'output_dir': '../win-<(target_arch)-bin',
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
		},
		
		# Don't assume a Cygwin environment when invoking actions
		'msvs_cygwin_shell': 0,
		
		'configurations':
		{
			'Debug':
			{
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
						'AdditionalOptions': '/NODEFAULTLIB:LIBCMT',
						'LinkIncremental': '2',
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
			'WINVER=0x0501',		# Windows XP
			'_WIN32_WINNT=0x0501',		# Windows XP
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
				'Detect64BitPortabilityProblems': 'false',
			},
			
			'VCLinkerTool':
			{
				'SubSystem': '2',
				'RandomizedBaseAddress': '1',	# /DYNAMICBASE:NO - disable ASLR
			},
		},
	},
}
