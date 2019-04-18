{
	'includes':
	[
		'../../common.gypi',
	],

    'variables':
    {
        'conditions':
        [
            [
                'host_os == "linux"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community',
                },
            ],
            [
                'host_os == "mac"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community',
                },
            ],
            [
                'host_os == "win"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community.exe',
                },
            ],
        ],
    },
    
	'targets':
	[
	    {
	        'target_name': 'extension-examples',
			'type': 'none',
			
			'dependencies':
			[
				'cross-platform-extension-dependencies',
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files':
					[
						# Gyp will only use a recursive xcopy on Windows if the path ends with '/'
						'<(PRODUCT_DIR)/packaged_extensions/',
					],
				},
			},
			
			'conditions':
			[
				[
            		'mobile == 0',
            		{
						'dependencies':
						[
							'lcs-extension-examples',
							'lcb-extension-examples',
						],	 
                    },
                ],
			],
	    },	    
		{
			'target_name': 'lcs-extension-examples',
			'type': 'none',
			
			'sources':
			[
			],
			
			'dependencies':
		    [
        		# Requires a working LiveCode engine
		        '../../engine/engine.gyp:server',
				'../../revzip/revzip.gyp:external-revzip-server',
				'../../revxml/revxml.gyp:external-revxml-server',		        
    		],
            
            'rules':
			[
				{
					'rule_name': 'build_script_extension',
					'extension': 'livecodescript',
					'message': 'Building script extension <(RULE_INPUT_NAME)',
										
					'outputs':
					[
						'<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.<(RULE_INPUT_ROOT)/<(RULE_INPUT_ROOT).livecodescript',	
					],
					          
					'action':
					[
						'<(engine)',
						'script-libraries/extension-utils/resources/extension-utils.lc',
						'$(not_a_real_variable)buildextension',
						'dummy1',
						'dummy2',
						'dummy3',
						'../../ide-support/revdocsparser.livecodescript',			
						'<(RULE_INPUT_DIRNAME)',
						'<(RULE_INPUT_ROOT).livecodescript',
						'<(PRODUCT_DIR)/packaged_extensions',
						'$(not_a_real_variable)false',
					],
				},
			],
		},
		
		{
			'target_name': 'lcb-extension-examples',
			'type': 'none',

			'dependencies':
			[
				'../../toolchain/lc-compile/lc-compile.gyp:lc-compile',
				'../../engine/lcb-modules.gyp:engine_lcb_modules',
				'../../engine/engine.gyp:server',
				'../../revzip/revzip.gyp:external-revzip-server',				
				'../../revxml/revxml.gyp:external-revxml-server',				
			],

			'sources':
			[
				'libraries/androidwavrecorder/androidwavrecorder.lcb',
			],

			'actions':
			[
				{
					'action_name': 'build_extensions',

					'inputs':
					[
						'../script-libraries/extension-utils/resources/extension-utils.lc',
						'<@(_sources)',
					],

					'outputs':
					[
						# hack because gyp wants an output
                        'notarealfile.txt',
					],

					'message': 'Building extensions',

					'action':
					[
						'<(engine)',
						'../script-libraries/extension-utils/resources/extension-utils.lc',
						'$(not_a_real_variable)buildlcbextensions',
												'../../ide-support/revdocsparser.livecodescript',
						'<(PRODUCT_DIR)/packaged_extensions',
						'$(not_a_real_variable)false',
						'>(lc-compile_host)',
						'<(PRODUCT_DIR)/modules/lci',
						'$(not_a_real_variable)',
						'<@(_sources)',
					],
				},
			],
		},
		
		{
			'target_name': 'cross-platform-extension-dependencies',
			'type': 'none',

			'dependencies':
			[
				'libraries/androidwavrecorder/src/androidwavrecorder.gyp:androidwavrecorder',			
			],
		},
	],
}
