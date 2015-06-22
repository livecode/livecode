{
	'variables':
	{
		'conditions':
		[
			[
				'host_os == "win"',
				{
					'perl': [ 'C:/perl/bin/perl.exe' ],
				},
				{
					'perl': [ 'perl' ],
				},
			],
		],
	},
}
