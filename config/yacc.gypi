{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "win"',
				{
					'flex': 'C:\cygwin\bin\flex.exe',
					'bison': 'C:\cygwin\bin\bison.exe',
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
