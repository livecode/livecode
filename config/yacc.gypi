{
	'variables':
	{
		'conditions':
		[
			[
				'OS == "win"',
				{
					'variables':
					{
						'invoke_unix_path': '$(ProjectDir)../../../../../util/invoke-unix.bat',
					},
					
					'flex': ['<(invoke_unix_path)', '/usr/bin/flex'],
					'bison': ['<(invoke_unix_path)', '/usr/bin/bison'],
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
