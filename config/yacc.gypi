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
						# Whether to use flex and bison installed via chocolatey winflexbison
						# package or installed through Cygwin
						'use_winflexbison': '<!(echo ${USE_WINFLEXBISON:-0})',
                    },

					'conditions':
					[
						[
							'use_winflexbison != "0"',
							{
								'flex': ['win_flex.exe'],
								'bison': ['win_bison.exe'],
							},
							{
			                    'variables':
			                    {
			                            'invoke_unix_path': '$(ProjectDir)../../../../../util/invoke-unix.bat',
			                    },

			                    'flex': ['<(invoke_unix_path)', '/usr/bin/flex'],
			                    'bison': ['<(invoke_unix_path)', '/usr/bin/bison'],
							},
						],
					],
				},
				{
					'flex': 'flex',
					'bison': 'bison',
				},
			],
		],
	},
}
