{
	'include':
	[
		'../common.gypi',
	],

	'targets':
	[
		{
			'target_name': 'host-server',
			'type': 'none',

			'toolsets': ['host', 'target'],

			'conditions':
			[
				[
					'cross_compile != 0',
					{
						'dependencies':
						[
							'engine.gyp:server#host',
						],

						'direct_dependent_settings':
						{
							'variables':
							{
								'engine': '<(PRODUCT_DIR)/server-community-host>(exe_suffix)'
							},
						},
					},
					{
						'dependencies':
						[
							'engine.gyp:server#target',
						],

						'direct_dependent_settings':
						{
							'variables':
							{
								'engine': '<(PRODUCT_DIR)/server-community>(exe_suffix)',
							},
						},
					},
				],
			],
		},
	],
}
