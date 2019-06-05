{
	'includes':
	[
		'common.gypi',
	],

	'targets':
	[
		{
			'target_name': 'LiveCode-Check-all',
			'type': 'none',

			'conditions':
			[
				[
					'mobile == 0',
					{
						'dependencies':
						[
							'livecode.gyp:check',
						],
					},
				],
			],
		},

	],
}
