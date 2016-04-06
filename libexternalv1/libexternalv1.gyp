{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libExternalV1',
			'type': 'static_library',
			
			'variables':
			{
				'library_for_module': 1,
			},
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'include/external.h',
				
				'src/external.c',
			],

			'msvs_settings':
			{
				'VCCLCompilerTool':
                        	{
                                	'ExceptionHandling': '1',
        	                        'RuntimeTypeInfo': 'true',
	                        },
			},

			'xcode_settings':
			{
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
				'GCC_ENABLE_CPP_RTTI': 'YES',
			},
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],

				'msvs_settings':
				{
					'VCCLCompilerTool':
					{
						'ExceptionHandling': '1',
						'RuntimeTypeInfo': 'true',
					},
				},

				'xcode_settings':
				{
					'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
					'GCC_ENABLE_CPP_RTTI': 'YES',
				},
			},
		},
	],
}
