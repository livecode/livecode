{
	'targets':
	[
		{
			'target_name': 'kernel-installer',
			'type': 'static_library',
			
			'dependencies':
			[
				'kernel',
				
				'../thirdparty/libz/libz.gyp:libz',
			],
			
			'includes':
			[
				'kernel-mode-template.gypi',
			],
			
			'variables':
			{
				'mode_macro': 'MODE_INSTALLER',
			},
			
			'sources':
			[
				'<@(engine_installer_mode_source_files)',
			],
			
			'conditions':
			[
				[
					'mobile != 0',
					{
						'type': 'none',
					},
				],
			],
		},
	],
}
