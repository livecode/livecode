{
	'includes':
	[
		'common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'LiveCode-all',
			'type': 'none',
			
			'dependencies':
			[
				# Engines
				'engine/engine.gyp:standalone',

				# The revsecurity library is an output and not an intermediate product
				'thirdparty/libopenssl/libopenssl.gyp:revsecurity',
				
				# Externals
				'revdb/revdb.gyp:external-revdb',
				'revdb/revdb.gyp:dbmysql',
				'revdb/revdb.gyp:dbsqlite',
				'revxml/revxml.gyp:external-revxml',
				'revzip/revzip.gyp:external-revzip',
			],
			
			'conditions':
			[
				[
					'mobile == 0',
					{
						'dependencies':
						[
							# Engines
							'engine/engine.gyp:development',
							'engine/engine.gyp:installer',
							'engine/engine.gyp:server',
							
							# Externals
							'revbrowser/revbrowser.gyp:external-revbrowser',
							'revbrowser/revbrowser.gyp:revbrowser-cefprocess',
							'revdb/revdb.gyp:dbodbc',
							'revdb/revdb.gyp:dbpostgresql',
							'revmobile/revmobile.gyp:external-revandroid',
							'revmobile/revmobile.gyp:external-reviphone',
							'revspeech/revspeech.gyp:external-revspeech',
							'revvideograbber/revvideograbber.gyp:external-revvideograbber',
							
							# Server externals
							'revdb/revdb.gyp:external-revdb-server',
						],
					},
				],
				[
					'OS == "mac" or OS == "win"',
					{
						'dependencies':
						[
							# Externals
							'revfont/revfont.gyp:external-revfont',
						],
					},
				],
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
				],
				[
					'OS == "ios"',
					{
						'dependencies':
						[
							'engine/engine.gyp:ios-standalone-executable',
						],
					},
				],
				[
					'OS != "android"',
					{
						'dependencies':
						[
							'revpdfprinter/revpdfprinter.gyp:external-revpdfprinter',
						],
					},
				],
			],
		},
		
		{
			'target_name': 'debug-symbols',
			'type': 'none',
			
			'dependencies':
			[
				'LiveCode-all',
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
				'LiveCode-all',
				'debug-symbols',
			],
			
			'copies':
			[{
				'destination': '<(output_dir)',
				'files': [ '>@(dist_files)', '>@(dist_aux_files)', ],
			}],
		},
	],
}
