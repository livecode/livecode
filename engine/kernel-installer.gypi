{
	'targets':
	[
		{
			'target_name': 'kernel-installer',
			'type': 'static_library',
			
			'dependencies':
			[
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
		},
	],
}
