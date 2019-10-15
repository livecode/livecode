{
	'includes':
	[
		'common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'default',
			'type': 'none',
			'dependencies': [ 'binzip-copy', ],
		},

		{
			'target_name': 'check',
			'type': 'none',
			'dependencies': [ 'cpptest-run-all', ],
		},

		{
			'target_name': 'LiveCode-all',
			'type': 'none',
			
			'dependencies':
			[
				# Engines
				'engine/engine.gyp:standalone',

				# LCB toolchain
				'toolchain/toolchain.gyp:toolchain-all',

				# Widgets and libraries
				'extensions/extensions.gyp:extensions',

				# lcidlc
				'lcidlc/lcidlc.gyp:lcidlc',
			],
			
			'conditions':
			[
				
				[
					'OS != "emscripten"',
					{
						'dependencies':
						[
							'thirdparty/libopenssl/libopenssl.gyp:revsecurity',
							'revdb/revdb.gyp:external-revdb',
							'revdb/revdb.gyp:dbmysql',
							'revdb/revdb.gyp:dbsqlite',
							'revxml/revxml.gyp:external-revxml',
							'revzip/revzip.gyp:external-revzip',
						],
					},
				],
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
							'revdb/revdb.gyp:dbodbc',
							'revdb/revdb.gyp:dbpostgresql',
							'revmobile/revmobile.gyp:external-revandroid',
							'revmobile/revmobile.gyp:external-reviphone',
							'revspeech/revspeech.gyp:external-revspeech',
							
							# Server externals
							'revdb/revdb.gyp:external-revdb-server',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'dependencies':
						[
							'Installer/osx-installer-stub.gyp:osx-installer-stub',
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
					'OS != "emscripten"',
					{
						'dependencies':
						[
							'revpdfprinter/revpdfprinter.gyp:external-revpdfprinter',
						],
					},
				],
				[
					'OS == "emscripten"',
					{
						'dependencies':
						[
							'engine/engine.gyp:javascriptify',
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
		
		{
			'target_name': 'cpptest-run-all',
			'type': 'none',

			'dependencies':
			[
				'libcpptest/libcpptest.gyp:run-test-libcpptest',
				'libfoundation/libfoundation.gyp:run-test-libFoundation',
				'engine/kernel-standalone.gyp:run-test-kernel-standalone',
			],

			'conditions':
			[
				[
					'mobile == 0',
					{
						'dependencies':
						[
							'engine/kernel-server.gyp:run-test-kernel-server',
							'engine/kernel-development.gyp:run-test-kernel-development',
							'engine/kernel-installer.gyp:run-test-kernel-installer',
						],
					},
				],
			],
		},
		
		{
			'target_name': 'thirdparty-prebuilts',
			'type': 'none',
			
			'variables':
			{
				'thirdparty_libs_core':
				[
					'thirdparty/libffi/libffi.gyp:libffi',
					'thirdparty/libpng/libpng.gyp:libpng',
					'thirdparty/libz/libz.gyp:libz',
					'thirdparty/libgif/libgif.gyp:libgif',
					'thirdparty/libjpeg/libjpeg.gyp:libjpeg',
					'thirdparty/libpcre/libpcre.gyp:libpcre',

					'thirdparty/libskia/libskia.gyp:libskia',
					'thirdparty/libskia/libskia.gyp:libskia_opt_none',
					'thirdparty/libskia/libskia.gyp:libskia_opt_arm',
					'thirdparty/libskia/libskia.gyp:libskia_opt_sse2',
					'thirdparty/libskia/libskia.gyp:libskia_opt_sse3',
					'thirdparty/libskia/libskia.gyp:libskia_opt_sse41',
					'thirdparty/libskia/libskia.gyp:libskia_opt_sse42',
					'thirdparty/libskia/libskia.gyp:libskia_opt_avx',
					'thirdparty/libskia/libskia.gyp:libskia_opt_hsw',
				],
				
				'thirdparty_libs_native':
				[
					'thirdparty/libmysql/libmysql.gyp:libmysql',
					'thirdparty/libsqlite/libsqlite.gyp:libsqlite',
					'thirdparty/libxml/libxml.gyp:libxml',
					'thirdparty/libxslt/libxslt.gyp:libxslt',
					'thirdparty/libzip/libzip.gyp:libzip',
				],
				
				'thirdparty_libs_desktop':
				[
					'thirdparty/libiodbc/libiodbc.gyp:libiodbc',
					'thirdparty/libpq/libpq.gyp:libpq',
					'thirdparty/libcairo/libcairo.gyp:libcairo',
				],
				
				'thirdparty_libs_fonts':
				[
					'thirdparty/libharfbuzz/libharfbuzz.gyp:libharfbuzz',
					'thirdparty/libfreetype/libfreetype.gyp:libfreetype',
				],
			},
			
			'dependencies':
			[
				'<@(thirdparty_libs_core)',
			],
			
			'conditions':
			[
				[
					'OS == "mac" or OS == "win" or OS == "linux"',
					{
						'dependencies':
						[
							'<@(thirdparty_libs_native)',
							'<@(thirdparty_libs_desktop)',
						],
					},
				],
				[
					'OS == "ios"',
					{
						'dependencies':
						[
							'<@(thirdparty_libs_native)',
							'thirdparty/libcairo/libcairo.gyp:libcairo',
						],
					},
				],
				[
					'OS == "android"',
					{
						'dependencies':
						[
							'<@(thirdparty_libs_native)',
							'<@(thirdparty_libs_fonts)',
							'thirdparty/libexpat/libexpat.gyp:libexpat',
							'thirdparty/libcairo/libcairo.gyp:libcairo',
						],
					},
				],
				[
					'OS == "emscripten"',
					{
						'dependencies':
						[
							'<@(thirdparty_libs_fonts)',
						],
					},
				],
			],
		},
	],
}
