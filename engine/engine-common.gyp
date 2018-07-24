{
	'includes':
	[
		'../common.gypi',
		'engine-sources.gypi',
	],

	'targets':
	[
		{
			'target_name': 'encode_version',
			'type': 'none',

			'toolsets': ['host', 'target'],
			
			'actions':
			[
				{
					'action_name': 'encode_version',
					'inputs':
					[
						'../util/encode_version.pl',
						'../version',
						'include/revbuild.h.in',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/include/revbuild.h',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/encode_version.pl',
						'-<(git_revision)',
						'.',
						'<(SHARED_INTERMEDIATE_DIR)',
					],
				},
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'<(SHARED_INTERMEDIATE_DIR)/include',
				],
			},
		},

		{
			'target_name': 'quicktime_stubs',
			'type': 'none',
			
			'toolsets': ['host', 'target'],

			'actions':
			[
				{
					'action_name': 'quicktime_stubs',
					'inputs':
					[
						'../util/weak_stub_maker.pl',
						'src/quicktime.stubs',
					],
					'outputs':
					[
						'<(SHARED_INTERMEDIATE_DIR)/src/quicktimestubs.mac.cpp',
					],
					
					'action':
					[
						'<@(perl)',
						'../util/weak_stub_maker.pl',
						'src/quicktime.stubs',
						'<@(_outputs)',
					],
				},
			],
		},

		{
			'target_name': 'security-community',
			'type': 'static_library',
			
			'dependencies':
			[
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',

				# Because our headers are so messed up...
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../libgraphics/libgraphics.gyp:libGraphics',
			],
			
			'sources':
			[
				'<@(engine_security_source_files)',
			],
		},

	],
}
