{

    'includes':
    [
        '../../../../common.gypi',
    ],
    
    'targets':
    [
        {
            'target_name': 'inih',
            'type': 'none',
            
            'conditions':
            [
        		[
                    'OS == "ios" and "iphoneos" in target_sdk',
                    {
                        'dependencies':
                        [
                            'inih-lcext',
                        ],
            		},
                    {

                        'dependencies':
                        [
                            'inih-build',
                        ],

                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.ini/code/<(platform_id)/',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/inih<(SHARED_LIB_SUFFIX)',
                                ],
                            },
                        ],
            		},
                ],
            ],     
        },
        {
            'target_name': 'inih-build',
            'product_prefix': '',
			'product_name': 'inih',
            'sources':
			[
				'ini.c',
                'ini.h',
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
            'target_name': 'inih-lcext',
            'product_prefix': '',
            'product_name': 'inih',
            'dependencies':
            [
                '../../../../toolchain/lc-compile/lc-compile.gyp:lc-compile',
                'inih-build',
            ],

            'type': 'none',

            'actions':
            [
                {
                    'action_name': 'ini_output_auxc',

                    'sources':
                    [
                        '../ini.lcb',
                    ],

                    'inputs':
                    [
                        '<(_sources)',
                    ],

                    'outputs':
                    [
                        '<(SHARED_INTERMEDIATE_DIR)/ini.cpp',
                    ],

                    'message': 'Output module as auxc',

                    'action':
                    [
                        '>(lc-compile_host)',
                        '--forcebuiltins',
                        '--modulepath',
                        '<(PRODUCT_DIR)/modules/lci',
                        '--outputauxc',
                        '<(SHARED_INTERMEDIATE_DIR)/ini.cpp',
                        '<(_sources)',
                    ],
                },
                {
                    'action_name': 'link_ini_lcext',

                    'inputs':
                    [
                        '../../../../tools/build-module-lcext-ios.sh',
                        '<(SHARED_INTERMEDIATE_DIR)/ini.cpp',
                        'ini.ios',
                        '<(PRODUCT_DIR)/inih<(STATIC_LIB_SUFFIX)',
                    ],

                    'outputs':
                    [
                        '<(PRODUCT_DIR)/inih.lcext',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.ini/code/<(platform_id)/module.lcm'
                    ],

                    'message': 'Link lcext',

                    'action':
                    [
                        '../../../../tools/build-module-lcext-ios.sh',
                        '<(SHARED_INTERMEDIATE_DIR)/ini.cpp',
                        'ini.ios',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.ini/code/<(platform_id)/inih.lcext',
                        '$(not_a_real_variable)com.livecode.library.ini',
                        '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.ini/code/<(platform_id)/module.lcm',
                        '<(PRODUCT_DIR)/inih<(STATIC_LIB_SUFFIX)',
                    ],
                },
                
            ],
        },
    ],

}
