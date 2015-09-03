{
	'variables':
	{
		'mobile': 0,
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
		[
			'OS == "emscripten"',
			{
				'includes':
				[
					'emscripten.gypi',
				],
			},
		],
	],
}
