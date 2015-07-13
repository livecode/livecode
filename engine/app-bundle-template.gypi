{
	'type': 'executable',
	
	'conditions':
	[
		[
			'OS == "mac" or OS == "ios"',
			{
				'target_conditions':
				[
					[
						'_type == "executable"',
						{
							'mac_bundle': 1,
						},
					],
				],
	
				'mac_bundle_resources':
				[
					'rsrc/LiveCode-Community.rsrc',
					'rsrc/English.lproj/Localisation.strings',
					'rsrc/LiveCodeDoc.icns',
					'rsrc/LiveCode.icns',
				],
	
				'xcode_settings':
				{
					'INFOPLIST_FILE': '<(app_plist)',	
				},
			}
		],
	],
}
