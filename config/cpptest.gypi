{
	'variables':
	{
		'module_test_dependencies%': '',
		'module_test_sources%': '',
		'module_test_include_dirs%': '',
		'module_test_defines%': '',
	},

	'targets': [
		{
			'target_name': 'test-<(module_name)',

			'conditions':
			[
				[
					'OS == "ios"',
					{
						# We can't compile to an executable, but we can at least compile.
						'type': 'static_library',
					},
					{
						'type': 'executable',
					}
				],
			],

			'dependencies':
			[
				'../libcpptest/libcpptest.gyp:libcpptest',
				'<@(module_test_dependencies)',
			],

			'sources':
			[
				'<@(module_test_sources)',
			],

			'include_dirs': [ '<@(module_test_include_dirs)', ],
			'defines': [ '<@(module_test_defines)', ],

			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': '1',	# /SUBSYSTEM:CONSOLE
				},
			},
		},

		{
			'target_name': 'run-test-<(module_name)',
			'type': 'none',

			'variables':
			{
				'javascriptify': '',
				'test_suffix': '',
				'exec_wrapper': '',
			},

			'dependencies': [ 'test-<(module_name)<(javascriptify)', ],

			'conditions':
			[
				[
					'OS == "emscripten"',
					{
						'variables':
						{
							'javascriptify': '-javascriptify',
							'test_suffix': '.js',
							'exec_wrapper': '<!(echo ${NODE_JS})',
						},
					},
				],
				[
					'OS == "win"',
					{
						'variables':
						{
							'test_suffix': '.exe',
						},
					},
				],
				[
					'OS == "ios" or OS == "android"',
					{
						# We can't run the tests on these platform so use `true`
						# as the wrapper
						'variables':
						{
							'exec_wrapper': 'true',
						},
					},
				],
			],

			'actions':
			[
				{
					'action_name': 'run-test-<(module_name)',
					'message': 'Running test-<(module_name)',

					'inputs': [ '<(PRODUCT_DIR)/test-<(module_name)<(test_suffix)', ],
					'outputs': [ '<(PRODUCT_DIR)/test-<(module_name).log' ],

					'action':
					[
						'<@(perl)',
						'../util/run-tests.pl',
						'<(exec_wrapper)',
						'<(PRODUCT_DIR)/test-<(module_name)<(test_suffix)',
						'<(PRODUCT_DIR)/test-<(module_name).log',
					],

				},
			],

		}
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
									'../util/emscripten-javascriptify.py',
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
									'../util/emscripten-javascriptify.py',
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
