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
					
					'flex': ['<(invoke_unix_path)', 'flex'],
					'bison': ['<(invoke_unix_path)', 'bison'],
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
