{
	'variables':
	{
		'mobile': 1,
		'output_dir': '../emscripten-bin',
	},

	'target_defaults':
	{
		'variables':
		{
			'app_bundle_suffix': '',
			'ext_bundle_suffix': '',
			'lib_suffix': '',
			'ext_suffix': '',
			'exe_suffix': '',
			'debug_info_suffix': '.dbg',

			'silence_warnings': 0,
		},

		'target_conditions':
		[
			[
				'_toolset == "host"',
				{
					'includes':
					[
						'linux-settings.gypi',
					],
				},
				{
					'includes':
					[
						'emscripten-settings.gypi',
					],
				},
			],
			[
				'_toolset == "target" and _type == "executable"',
				{
					'product_extension': 'bc',
				},
			],
		],
	},
}
