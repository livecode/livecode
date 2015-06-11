{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "win"',
				{
					'flex': ['../util/invoke-unix.bat', 'flex'],
					'bison': ['../util/invoke-unix.bat', 'bison'],
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
