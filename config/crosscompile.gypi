{
	'variables':
	{
		'variables':
		{
			'conditions':
			[	
				[
					# Cross-compilation not supported for Windows and OSX targets
					'OS == "win" or OS == "mac"',
					{
						'cross_compile': 0,
					},
				],
				[
					# iOS builds are always cross-compiled
					'OS == "ios"',
					{
						'cross_compile': 1,
					},
				],
				[
					# Android builds are always cross-compiled
					'OS == "android"',
					{
						'cross_compile': 1,
					},
				],
				[
					# Linux may or may not be cross-compiled
					'OS == "linux"',
					{
						# Default to not cross-compiling
						'cross_compile%': 0,
					},
				],
			],
		},
	},
}
