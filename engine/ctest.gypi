{
	'targets':
	[
		{
			'target_name': 'engine-ctest',
			'type': 'executable',
			
			'dependencies':
			[
				'../ctest/ctest.gyp:libctest',
				'kernel-server',
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../libgraphics/libgraphics.gyp:libGraphics',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],

			'sources':
			[
				'<!@(ls -1 ctest/*.cpp)',

				'<@(engine_security_source_files)',
				# There must be a better way to provide the required symbols
			],
			
			# This is a huge hack to allow multiple definitions of main()
			'ldflags':
			[
				'-zmuldefs',
			]
		},
	],
}
