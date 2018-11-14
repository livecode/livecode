{

    'includes':
    [
        '../../../../common.gypi',
    ],

    'variables':
    {
        'zic': '<(PRODUCT_DIR)/zic<(EXECUTABLE_SUFFIX)',
    },

    'targets':
    [
        {
            'target_name': 'tz',
            'type': 'none',
            
            'conditions':
            [
            	[
            		'OS == "mac" or OS == "linux"',
            		{
            			'dependencies':
            			[
            		    	'tzdata',
            		    ],
            		}
            	],
        		[
                    'OS == "ios" and "iphoneos" in target_sdk',
                    {
                        'dependencies':
						[
							'libtz-lcext',
						],
            		},
                    {

						'dependencies':
						[
							'libtz-build',
						],
            
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/code/<(platform_id)/',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/tz<(SHARED_LIB_SUFFIX)',
                                ],
                            },
                        ],
            		},
                ],
            ],     
        },
		{
            'target_name': 'tzdata',
			'type': 'none', 
						
		    'dependencies':
            [
                'zic-build',
                'tzdata-build',
            ],
            
			'variables':
			{
				'install_folder': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/resources/zoneinfo',
			},            
            
			'actions':
			[
				{
					'action_name': 'tzdata',

					'inputs':
					[
						'<(zic)',
						'<(SHARED_INTERMEDIATE_DIR)/tzdata.zi',
					],

					'outputs':
					[
                        '<(install_folder)/GMT',
					],

					'message': 'Extracting zoneinfo',
					
					'action':
					[
						'<(zic)',
						'-d',
						'<(install_folder)',
						'<(SHARED_INTERMEDIATE_DIR)/tzdata.zi',
					],
				},
			],
		},        
        {
            'target_name': 'tzdata-build',
			'type': 'none', 
				
            'sources':
			[
				'africa',
				'antarctica',
				'asia',
				'australasia',
				'europe',
				'northamerica',
				'southamerica',
				'etcetera',
				'systemv',
				'factory',
			],	        
			
			'actions':
			[
				{
					'action_name': 'tzdata_zi',

					'inputs':
					[
						'<@(_sources)',
						'ziguard.awk',
					],

					'outputs':
					[
                        '<(SHARED_INTERMEDIATE_DIR)/tzdata.zi',
					],

					'message': 'Building zoneinfo',

					'conditions':
					[
						[
							'OS == "win"',
							{
								'variables':
								{
									'build_command': [ '$(ProjectDir)../../../../../../util/invoke-unix.bat', '$(ProjectDir)../../../../../../extensions/libraries/timezone/tz/tzdata.sh' ],
								},
							},
							{
								'variables':
								{
									'build_command': [ '../../../../extensions/libraries/timezone/tz/tzdata.sh' ],
								},
							},
						],
						[
						    'OS == "linux"',
						    {
						        'variables':
						        {
						            'awk': 'gawk',
						        },
						    },
						    {
						        'variables':
						        {
						            'awk': 'awk',
						        },						    
						    },
						]
					],
					
					'action':
					[
						'<@(build_command)',
						'<(SHARED_INTERMEDIATE_DIR)',
						'<(awk)',
						'ziguard.awk',
						'zishrink.awk',
						'version',
						'<@(_sources)',
					],
				},
			],
		},
        {
            'target_name': 'zic-build',
			'type': 'executable',
			'product_name': 'zic',      
			
			'variables':
			{			
				'silence_warnings': 1,
			},    
				
            'sources':
			[
				'zic.c',
			],			 
        },
        {
            'target_name': 'libtz-build',
            'product_prefix': '',
			'product_name': 'tz',
			
			'variables':
			{			
				'silence_warnings': 1,
			},    
			
            'sources':
			[
				'tz.h',
				'asctime.c',
				'private.h',
				'localtime.c',	
			],
            
            'conditions':
            [
        		[
                    # loadable_module is overridden for iOS so hack around that
                    'OS == "ios" and "iphoneos" in target_sdk',
                    {
                        'type': 'static_library',
            		},
                    'OS == "ios" and "iphoneos" not in target_sdk',
                    {
                        'type': 'shared_library',
            		},
                    {
                        'type': 'loadable_module',
            		},
                ],
            ],     
        },
        {
            'target_name': 'libtz-lcext',
            'product_prefix': '',
            'product_name': 'tz',
            'dependencies':
            [
                '../../../../toolchain/lc-compile/lc-compile.gyp:lc-compile',
                'libtz-build',
            ],

            'type': 'none',

            'actions':
            [
                {
                    'action_name': 'timezone_output_auxc',

                    'sources':
                    [
                        '../timezone.lcb',
                    ],

                    'inputs':
                    [
                        '<(_sources)',
                    ],

                    'outputs':
                    [
                        '<(SHARED_INTERMEDIATE_DIR)/timezone.cpp',
                    ],

                    'message': 'Output module as auxc',

                    'action':
                    [
                        '>(lc-compile_host)',
                        '--forcebuiltins',
                        '--modulepath',
                        '<(PRODUCT_DIR)/modules/lci',
                        '--outputauxc',
                        '<(SHARED_INTERMEDIATE_DIR)/timezone.cpp',
                        '<(_sources)',
                    ],
                },
                {
                    'action_name': 'link_timezone_lcext',

                    'inputs':
                    [
                        '../../../../tools/build-module-lcext-ios.sh',
                        '<(SHARED_INTERMEDIATE_DIR)/timezone.cpp',
                        'tz.ios',
                        '<(PRODUCT_DIR)/tz<(STATIC_LIB_SUFFIX)',
                    ],

                    'outputs':
                    [
                        '<(PRODUCT_DIR)/tz.lcext',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/code/<(platform_id)/module.lcm'
                    ],

                    'message': 'Link lcext',

                    'action':
                    [
                        '../../../../tools/build-module-lcext-ios.sh',
                        '<(SHARED_INTERMEDIATE_DIR)/timezone.cpp',
                        'tz.ios',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/code/<(platform_id)/tz.lcext',
                        '$(not_a_real_variable)com.livecode.library.timezone',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/code/<(platform_id)/module.lcm',
                        '<(PRODUCT_DIR)/tz<(STATIC_LIB_SUFFIX)',
                    ],
                },
                
            ],
        },
    ],

}
