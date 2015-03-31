{
	'targets':
	[
		{
			'target_name': 'kernel-standalone',
			'type': 'static_library',
			
			'includes':
			[
				'kernel-mode-template.gypi',
			],
			
			'variables':
			{
				'mode_macro': 'MODE_STANDALONE',
			},
			
			'sources':
			[
				'<@(engine_standalone_mode_source_files)',
			],
		},
	],
}
