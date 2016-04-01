{
	'includes':
	[
		'../common.gypi',
	],

	'targets':
	[
		{
			'target_name': 'osx-installer-stub',
			'product_name': 'installer-stub',
			'type': 'executable',
			'mac_bundle': '0',
			
			'sources':
			[
				'osx-installer-stub.mm',
			],

			'libraries':
			[
				'$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
			],

			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(exe_suffix)' ],
				},
			},
		},
	],
}

