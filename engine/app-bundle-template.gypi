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
					'rsrc/Dutch.lproj/Localisation.strings',
					'rsrc/English.lproj/Localisation.strings',
					'rsrc/French.lproj/Localisation.strings',
					'rsrc/German.lproj/Localisation.strings',
					'rsrc/Italian.lproj/Localisation.strings',
					'rsrc/Japanese.lproj/Localisation.strings',
					'rsrc/Spanish.lproj/Localisation.strings',
					'rsrc/ar.lproj/Localisation.strings',
					'rsrc/da.lproj/Localisation.strings',
					'rsrc/fi.lproj/Localisation.strings',
					'rsrc/he.lproj/Localisation.strings',
					'rsrc/ko.lproj/Localisation.strings',
					'rsrc/no.lproj/Localisation.strings',
					'rsrc/pt.lproj/Localisation.strings',
					'rsrc/ru.lproj/Localisation.strings',
					'rsrc/sv.lproj/Localisation.strings',
					'rsrc/zh_CN.lproj/Localisation.strings',
					'rsrc/zh_TW.lproj/Localisation.strings',
                    'rsrc/terminology.sdef',
				],
	
				'xcode_settings':
				{
					'INFOPLIST_FILE': '<(app_plist)',	
				},
			}
		],
	],
}
