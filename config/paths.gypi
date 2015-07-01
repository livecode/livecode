{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "linux" or OS == "android" or OS == "emscripten"',
				{
					'src_top_dir_abs': '$(abs_srcdir)',
				},
			],
		],
	},
}
