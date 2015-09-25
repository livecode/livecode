{
	'targets': [
		{
			'target_name': 'test-<(module_name)',
			'type': 'executable',

		  'dependencies':
			[
				'libcpptest'
			],

			'sources':
			[
				'<!@(ls -1 test/*.cpp)',
			],

			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},

		},
	],

 	'conditions':
	[
		[
			'OS == "emscripten"',
			{
				'targets':
				[
					{
						'target_name': 'test-<(module_name)-javascriptify',
						'type': 'none',

						'dependencies':
						[
							'test-<(module_name)',
						],

						'actions':
						[
							{
								'action_name': 'test-<(module_name)-javascriptify',
								'message': 'Javascript-ifying test-<(module_name)',

								'inputs':
								[
									'../engine/emscripten-javascriptify.py',
									'<(PRODUCT_DIR)/test-<(module_name).bc',
								],

								'outputs':
								[
									'<(PRODUCT_DIR)/test-<(module_name).js',
									'<(PRODUCT_DIR)/test-<(module_name).html',
									'<(PRODUCT_DIR)/test-<(module_name).html.mem',
								],

								'action':
								[
									'../engine/emscripten-javascriptify.py',
									'--input',
									'<(PRODUCT_DIR)/test-<(module_name).bc',
									'--output',
									'<(PRODUCT_DIR)/test-<(module_name).html',
								],
							},
						],
					},
				],
			},
		],
	],
}