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
				'revdb/revdb.gyp:revdb',
				'revdb/revdb.gyp:dbmysql',
				'revdb/revdb.gyp:dbsqlite',
				'revxml/revxml.gyp:revxml',
				'revzip/revzip.gyp:revzip',
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
							'revbrowser/revbrowser.gyp:revbrowser',
							'revbrowser/revbrowser.gyp:revbrowser-cefprocess',
							'revdb/revdb.gyp:dbodbc',
							'revdb/revdb.gyp:dbpostgresql',
							'revfont/revfont.gyp:revfont',
							'revmobile/revmobile.gyp:revandroid',
							'revmobile/revmobile.gyp:reviphone',
							'revspeech/revspeech.gyp:revspeech',
							'revvideograbber/revvideograbber.gyp:revvideograbber',
							
							# Server externals
							'revdb/revdb.gyp:revdb-server',
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
							'revxml/revxml.gyp:revxml-server',
							'revzip/revzip.gyp:revzip-server',
						],
					},
				],
				[
					'OS == "ios"',
					{
						'dependencies':
						[
							'engine/engine.gyp:standalone-mobile-lib-community',
						],
					},
				],
				[
					'OS != "android"',
					{
						'dependencies':
						[
							'revpdfprinter/revpdfprinter.gyp:revpdfprinter',
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
			
			'conditions':
			[
				[
					'OS == "linux" or OS == "android"',
					{
						'variables':
						{
							'debug_symbol_files':
							[
								'>!@(["sh", "-c", "echo $@ | xargs -n1 | sed -e \\\"s/$/>(debug_info_suffix)/g\\\"", "echo", \'>@(dist_files)\'])',
							],
						},
			
						'actions':
						[
							{
								'action_name': 'extract-debug-symbols',
								'message': 'Extracting debug symbols',
					
								'inputs':
								[
									'>@(dist_files)',
									'./tools/extract-debug-symbols.sh',
								],
					
								'outputs':
								[
									'<@(debug_symbol_files)',
								],
					
								'action':
								[
									'./tools/extract-debug-symbols.sh',
									'>(debug_info_suffix)',
									'<@(_inputs)',
								],
							},
						],
			
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_aux_files': [ '<@(debug_symbol_files)' ],
							},
						},
					}
				],
			],
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
