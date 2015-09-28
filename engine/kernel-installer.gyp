{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
	],

	'targets':
	[
		{
			'target_name': 'kernel-installer',
			'type': 'static_library',
			
			'dependencies':
			[
				'kernel.gyp:kernel',
				
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
				'<@(engine_minizip_source_files)',
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
