{
	'variables':
	{
		'mobile': 0,
		'src_top_dir_abs': '',	# Only needed for Linux and Android
	},
	
	'conditions':
	[
		[
			'OS == "mac"',
			{
				'includes':
				[
					'mac.gypi',
				],
			},
		],
		[
			'OS == "linux"',
			{
				'includes':
				[
					'linux.gypi',
				],
			},
		],
		[
			'OS == "android"',
			{
				'includes':
				[
					'android.gypi',
				],
			},
		],
		[
			'OS == "win"',
			{
				'includes':
				[
					'win32.gypi',
				],
			},
		],
		[
			'OS == "ios"',
			{
				'includes':
				[
					'ios.gypi',
				],
			},
		],
	],
}
