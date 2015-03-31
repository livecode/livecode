{
	'targets':
	[
		{
			'target_name': 'kernel-development',
			'type': 'static_library',
			
			'includes':
			[
				'kernel-mode-template.gypi',
			],
			
			'variables':
			{
				'mode_macro': 'MODE_DEVELOPMENT',
			},
			
			'dependencies':
			[
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl',
				'../thirdparty/libz/libz.gyp:libz',
			],
			
			'sources':
			[
				'<@(engine_development_mode_source_files)',
			],
		},
	],
}
