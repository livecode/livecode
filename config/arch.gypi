{
	'variables':
	{
		# Nested variables are a bit of a strange beast in Gyp; they are
		# evaluated before the containing dictionary and are therefore
		# the best way to have variables that are conditional on other
		# variables...
		'variables':
		{
			'variables':
			{
				'conditions':
				[
					[
						'OS == "win"',
						{
							'uname_m': 'x86',
							'uname_s': 'win',
						},
						{
							'uname_m': '<!(uname -m)',
							'uname_s': '<!(uname -s)',
						},
					],
				],
			},
		
			'conditions':
			[
				[
					'uname_m == "x86" or uname_m == "i386" or uname_m == "i486" or uname_m == "i586" or uname_m == "i686"',
					{
						'host_arch': 'x86',
					},
				],
				[
					'uname_m == "x86_64" or uname_m == "amd64"',
					{
						'host_arch': 'x86_64',
					},
				],
				[
					'OS == "mac" or OS == "ios" or uname_s == "Darwin"',
					{
						'host_os': 'mac',
					},
				],
				[
					'uname_s == "win"',
					{
						'host_os': 'win',
					},
				],
				[
					'OS != "mac" and OS != "ios" and uname_s != "Darwin" and uname_s != "win"',
					{
						'host_os': 'linux',
					},
				],
				[
					'OS == "emscripten"',
					{
						'target_arch': 'js',
					},
				],
			],
		},
		
		'target_arch%': '<(host_arch)',
		'host_arch': '<(host_arch)',
		'host_os': '<(host_os)',
	},
}
