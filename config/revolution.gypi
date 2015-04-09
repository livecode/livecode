{
	'variables':
	{
		'conditions':
		[
			# Location of the prebuilt engine
			[
				'host_os == "mac"',
				{
					'revolution_path': '../prebuilt/bin/Revolution.osx',
					'perfect_path': '../prebuilt/bin/perfect.osx',
				},
			],
			[
				'host_os == "win"',
				{
					'revolution_path': '$(SolutionDir)/../prebuilt/bin/Revolution.exe',
					'perfect_path': '$(SolutionDir)/../prebuilt/bin/perfect.exe',
				},
			],
			[
				'host_os == "linux"',
				{
					'revolution_path': '../prebuilt/bin/Revolution.lnx',
					'perfect_path': '../prebuilt/bin/perfect.lnx',
				},
			],
		],
	},
}
