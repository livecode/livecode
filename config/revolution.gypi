{
	'variables':
	{
		'conditions':
		[
			# Location of the prebuilt engine
			[
				'OS == "mac" or OS == "ios" or OS == "android"',
				{
					'revolution_path': '../prebuilt/bin/Revolution.osx',
					'perfect_path': '../prebuilt/bin/perfect.osx',
				},
			],
			[
				'OS == "win"',
				{
					'revolution_path': '$(SolutionDir)/../prebuilt/bin/Revolution.exe',
					'perfect_path': '$(SolutionDir)/../prebuilt/bin/perfect.exe',
				},
			],
			[
				'OS == "linux"',
				{
					'revolution_path': '../prebuilt/bin/Revolution.lnx',
					'perfect_path': '../prebuilt/bin/perfect.lnx',
				},
			],
		],
	},
}
