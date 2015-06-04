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
		
		'conditions':
		[
			[
				'cross_compile != 0',
				{
					'host_toolset_var': ['host'],
					'both_toolset_var': ['host','target'],
				},
				{
					'host_toolset_var': ['target'],
					'both_toolset_var': ['target'],
				},
			],
		],
	},
	
	'target_defaults':
	{
		'variables':
		{
			'host_and_target': '0',
			'host_only': '0',
		},
		
		'target_conditions':
		[
			[
				'host_and_target != 0',
				{
					'toolsets': ['<@(both_toolset_var)'],
				},
			],
			[
				'host_and_target == 0 and host_only != 0',
				{
					'toolsets': ['<@(host_toolset_var)'],
				},
			],
		],
	},
}
