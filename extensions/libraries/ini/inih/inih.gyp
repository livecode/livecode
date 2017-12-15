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
            
            'dependencies':
            [
                'inih-build',
            ],
            
            'conditions':
            [
        		[
                    'OS == "ios" and "iphoneos" in target_sdk',
                    {
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.ini/code/<(platform_id)/',
                                'files':
                                [
                                    '<(PRODUCT_DIR)/inih<(STATIC_LIB_SUFFIX)',
                                ],
                            },
                        ],
            		},
                    {
                    
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
    ],

}
