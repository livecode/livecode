{
	'target_defaults':
	{
		'suppress_warnings': 0,
		
		'target_conditions':
		[
			[
				'toolset_os != "win" and _suppress_warnings != 0',
				{
					'cflags_c':
					[
						'-w',
						'-Wno-error',
						'-Wno-return-type',
					],
					
					'cflags_cc':
					[
						'-w',
						'-Wno-error',
						'-Wno-return-type',
					],
					
					'xcode_settings':
					{
						'OTHER_CFLAGS':
						[
							'-w',
							'-Wno-error',
							'-Wno-return-type',
						],
					},
				},
			],
		],
	},
}
