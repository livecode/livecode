{
	'variables':
	{
		'module_name': 'kernel-installer',
		'module_test_dependencies':
		[
			'kernel-installer',
			'engine-common.gyp:security-community',
			'../libfoundation/libfoundation.gyp:libFoundation',
			'../libgraphics/libgraphics.gyp:libGraphics',
		],
		'module_test_include_dirs':
		[
			'include',
			'src',
		],
		'module_test_defines': [ 'MODE_INSTALLER', ],
		'module_test_sources':
		[
			'<@(engine_test_source_files)',
		],
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
			'target_name': 'kernel-installer',
			'type': 'static_library',
			
			'dependencies':
			[
				'kernel.gyp:kernel',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_z',
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
