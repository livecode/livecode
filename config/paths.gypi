{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "linux" or OS == "android"',
				{
					'src_top_dir_abs': '$(abs_srcdir)',
				},
			],
		],
	},
}
