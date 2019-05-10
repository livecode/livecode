{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "win"',
				{
					'flex': ['win_flex.exe'],
					'bison': ['win_bison.exe'],
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
