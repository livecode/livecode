{
	'includes':
	[
		'../common.gypi',
	],
	
	'variables':
	{
		'revdb_sources':
		[
			'src/revdb.cpp',
			'src/osxsupport.cpp',
			'src/unxsupport.cpp',
			'src/w32support.cpp',
			'src/database.cpp',
			'src/dbdrivercommon.cpp',
		],
	},
	
	'target_defaults':
	{
		'conditions':
		[	[
				'OS == "mac" or OS == "win"',
				{
					'sources!':
					[
						'src/unxsupport.cpp',
					],
				},
			]
		],
	},
	
	'targets':
	[
		{
			'target_name': 'dbmysql',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libmysql/libmysql.gyp:libmysql',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl',
				'../thirdparty/libz/libz.gyp:libz',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/dbdrivercommon.cpp',
				'src/database.cpp',
				'src/dbmysqlapi.cpp',
				'src/mysql_connection.cpp',
				'src/mysql_cursor.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbmysql-Info.plist',
			},
			
			'variables':
			{
				'ios_external_symbols':
				[
					'_setidcounterref',
					'_newdbconnectionref',
					'_releasedbconnectionref',
				],
			},
		},
		{
			'target_name': 'dbodbc',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			# Not for Windows
			'conditions':
			[
				[
					'OS == "win"',
					{
						'type': 'none',
					},
				],
			],
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libiodbc/libiodbc.gyp:libiodbc',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/dbdrivercommon.cpp',
				'src/database.cpp',
				'src/dbodbcapi.cpp',
				'src/odbc_connection.cpp',
				'src/odbc_cursor.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbodbc-Info.plist',
			},
			
			'variables':
			{
				'ios_external_symbols':
				[
					'_setidcounterref',
					'_newdbconnectionref',
					'_releasedbconnectionref',
				],
			},
		},
		{
			'target_name': 'dbpostgresql',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libpq/libpq.gyp:libpq',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/dbdrivercommon.cpp',
				'src/database.cpp',
				'src/dbpostgresqlapi.cpp',
				'src/postgresql_connection.cpp',
				'src/postgresql_cursor.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbpostgresql-Info.plist',
			},
			
			'variables':
			{
				'ios_external_symbols':
				[
					'_setidcounterref',
					'_newdbconnectionref',
					'_releasedbconnectionref',
				],
			},
		},
		{
			'target_name': 'dbsqlite',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libsqlite/libsqlite.gyp:libsqlite',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/dbdrivercommon.cpp',
				'src/database.cpp',
				'src/dbsqliteapi.cpp',
				'src/sqlite_connection.cpp',
				'src/sqlite_cursor.cpp',
			],
			
			'msvs_settings':
			{
				'VCCLCompilerTool':
				{
					'ExceptionHandling': 1,
				},
			},
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbsqlite-Info.plist',
				'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
			},
			
			'variables':
			{
				'ios_external_symbols':
				[
					'_setidcounterref',
					'_newdbconnectionref',
					'_releasedbconnectionref',
				],
			},
			
			'conditions':
			[
				[
					'OS == "linux" or OS == "android"',
					{
						'cflags_cc':
						[
							'-fexceptions',
						],
					},
				],
			],
		},
		{
			'target_name': 'revdb',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(revdb_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revdb-Info.plist',
			},
			
			'variables':
			{
				'ios_external_symbols': [ '_getXtable' ],
			},
		},
		{
			'target_name': 'revdb-server',
			'type': 'loadable_module',
			'product_prefix': '',
			
			'variables':
			{
				'server_mode': 1,
				'ios_external_symbols': [],
			},
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(revdb_sources)',
			],
		},
	],
}
