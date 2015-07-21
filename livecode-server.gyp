{
	'includes':
	[
		'common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'LiveCode-server-all',
			'type': 'none',
			
			'dependencies':
			[
				# LCB toolchain
				'toolchain/toolchain.gyp:toolchain-all',
				
				# Engines
				'engine/engine.gyp:server',

				# Widgets and libraries
				'extensions/extensions.gyp:extensions',
			],
			
			'conditions':
			[
				[
					# Server builds use special externals on OSX and Linux
					'OS == "mac" or OS == "linux"',
					{
						'dependencies':
						[
							'revdb/revdb.gyp:dbmysql-server',
							'revdb/revdb.gyp:dbodbc-server',
							'revdb/revdb.gyp:dbpostgresql-server',
							'revdb/revdb.gyp:dbsqlite-server',
							'revxml/revxml.gyp:external-revxml-server',
							'revzip/revzip.gyp:external-revzip-server',
						],
					},
					{
						'dependencies':
						[
							'revdb/revdb.gyp:dbmysql',
							'revdb/revdb.gyp:dbodbc',
							'revdb/revdb.gyp:dbpostgresql',
							'revdb/revdb.gyp:dbsqlite',
							'revxml/revxml.gyp:external-revxml',
							'revzip/revzip.gyp:external-revzip',
						]
					},
				],
			],
		},
		
		{
			'target_name': 'debug-symbols',
			'type': 'none',
			
			'dependencies':
			[
				'LiveCode-server-all',
			],
			
			'variables':
			{
				'debug_syms_inputs%': [ '<@(debug_syms_inputs)' ],
				'variables':
				{
					'debug_syms_inputs': [ '>@(dist_files)' ],
				},
			},
			
			'includes':
			[
				'config/debug_syms.gypi',
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files': [ '<@(debug_syms_outputs)' ],
					'variables':
					{
						'debug_syms_inputs%': [ '<@(debug_syms_inputs)' ],
					},
				},
			},
		},
		
		{
			'target_name': 'binzip-copy',
			'type': 'none',
			
			'variables':
			{
				'dist_files': [],
				'dist_aux_files': [],
			},
			
			'dependencies':
			[
				'LiveCode-server-all',
				'debug-symbols',
			],
			
			'copies':
			[{
				'destination': '<(output_dir)',
				'files': [ '>@(dist_files)', '>@(dist_aux_files)', ],
			}],
		},

		{
			'target_name': 'install',
			'type': 'none',

			'variables':
			{
				'prefix': '/opt/livecode',
			},

			'dependencies':
			[
				'binzip-copy',
			],

			'actions':
			[
				{
					'action_name': 'install',
					'inputs': [ 'tools/install-server-linux.sh' ],
					'outputs': [ '.installed' ],
					'action': [ 'tools/install-server-linux.sh', '<(output_dir)', '<(prefix)' ],
				},
			],
		},
	],
}
