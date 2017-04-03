{
	'variables':
	{
		'module_name': 'kernel-development',
		'module_test_dependencies':
		[
			'kernel-development',
			'engine-common.gyp:security-community',
			'../libfoundation/libfoundation.gyp:libFoundation',
			'../libgraphics/libgraphics.gyp:libGraphics',
			'libplatform.gyp:libplatform',
		],
		'module_test_sources':
		[
			'<@(engine_test_source_files)',
			'<(SHARED_INTERMEDIATE_DIR)/src/startupstack.cpp',
		],
		'module_test_include_dirs':
		[
			'include',
			'src',
		],
		'module_test_defines': [ 'MODE_DEVELOPMENT', ],
	},

	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
		'../config/cpptest.gypi'
	],

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
				'kernel.gyp:kernel',

				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',
				'../thirdparty/libz/libz.gyp:libz',
			],
			
			'sources':
			[
				'<@(engine_development_mode_source_files)',
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
