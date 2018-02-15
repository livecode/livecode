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
            
            'dependencies':
            [
                'libtz-build',
                'tzdata',
            ],
            
            'conditions':
            [
        		[
                    'OS == "ios" and "iphoneos" in target_sdk',
                    {
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.timezone/code/<(platform_id)/',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/tz<(STATIC_LIB_SUFFIX)',
                                ],
                            },
                        ],
            		},
                    {
                    
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
						'<(INTERMEDIATE_DIR)/tzdata.zi',
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
						'<(INTERMEDIATE_DIR)/tzdata.zi',
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
                        '<(INTERMEDIATE_DIR)/tzdata.zi',
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
					],
					
					'action':
					[
						'<@(build_command)',
						'<(INTERMEDIATE_DIR)',
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
				'difftime.c',
				'localtime.c',	
				'strftime.c',
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
    ],

}
