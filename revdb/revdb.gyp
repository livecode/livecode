{
	'includes':
	[
		'../common.gypi',
	],
	
	'variables':
	{
		'revdb_sources':
		[
            'src/revdb.h',
            'src/revdbapi.h',
            'src/database.h',
            'src/dbcursor.h',
            'src/dbdriver.h',
            'src/iossupport.h',
            'src/osxsupport.h',
            'src/unxsupport.h',
            'src/w32support.h',
            'src/large_buffer.h',
        
			'src/revdb.cpp',
			'src/iossupport.cpp',
			'src/osxsupport.cpp',
			'src/unxsupport.cpp',
			'src/w32support.cpp',
			'src/database.cpp',
			'src/dbdrivercommon.cpp',
		],
		
		'dbmysql_sources':
		[
            'src/dbmysql.h',
			'src/dbdrivercommon.cpp',
			'src/database.cpp',
			'src/dbmysqlapi.cpp',
			'src/mysql_connection.cpp',
			'src/mysql_cursor.cpp',
			'src/ssl.cpp',
		],
		
		'dbodbc_sources':
        [
            'src/dbodbc.h',
			'src/dbdrivercommon.cpp',
			'src/database.cpp',
			'src/dbodbcapi.cpp',
			'src/odbc_connection.cpp',
			'src/odbc_cursor.cpp',
		],
		
		'dbpostgresql_sources':
        [
            'src/dbpostgresql.h',
			'src/dbdrivercommon.cpp',
			'src/database.cpp',
			'src/dbpostgresqlapi.cpp',
			'src/postgresql_connection.cpp',
			'src/postgresql_cursor.cpp',
			'src/ssl.cpp',
		],
		
		'dbsqlite_sources':
		[
            'src/dbsqlite.h',
			'src/dbdrivercommon.cpp',
			'src/database.cpp',
			'src/dbsqliteapi.cpp',
			'src/sqlite_connection.cpp',
			'src/sqlite_cursor.cpp',
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
			'product_name': 'dbmysql',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_mysql',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbmysql_sources)',
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
					'_setcallbacksref',
				],
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'product_name': 'DbMysql',
						'product_extension': '',
					},
				],
			],
		},
		{
			'target_name': 'dbmysql-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-dbmysql',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_mysql',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbmysql_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbmysql-Info.plist',
			},
			
			'variables':
			{
				'server_mode': 1,
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
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbodbc',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'dbodbc',
			
			# Requires libiodbc on non-Windows targets
			'conditions':
			[
				[
					'OS != "win"',
					{
						'dependencies':
						[
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_iodbc',
						],
					},
					{
						'libraries':
						[
							'-lodbc32.lib',
						],
					},
				],
			],
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbodbc_sources)',
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
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbodbc-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-dbodbc',
			
			# Requires libiodbc on non-Windows targets
			'conditions':
			[
				[
					'OS != "win"',
					{
						'dependencies':
						[
							'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_iodbc',
						],
					},
					{
						'libraries':
						[
							'-lodbc32.lib',
						],
					},
				],
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbodbc_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbodbc-Info.plist',
			},
			
			'variables':
			{
				'server_mode': 1,
				'ios_external_symbols':
				[
					'_setidcounterref',
					'_newdbconnectionref',
					'_releasedbconnectionref',
				],
			},
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbpostgresql',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'dbpostgresql',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_pq',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbpostgresql_sources)',
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
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbpostgresql-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-dbpostgresql',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_pq',
				'../thirdparty/libopenssl/libopenssl.gyp:libopenssl_stubs',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbpostgresql_sources)',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/dbpostgresql-Info.plist',
			},
			
			'variables':
			{
				'server_mode': 1,
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
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],

			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbsqlite',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'dbsqlite',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_sqlite',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbsqlite_sources)',
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
					'OS != "win"',
					{
						'cflags_cc!':
						[
							'-fno-exceptions',
						],
					},
				],
				[
					'OS == "android"',
					{
						'product_name': 'DbSqlite',
						'product_extension': '',
						'cflags!':
						[
							# Error in ../../thirdparty/libsqlite/include/qry_dat.h
							'-Werror=return-type',
						],
						'libraries':
						[
							'-lm',
						],
					},
				],
				[
					'OS == "linux"',
					{
						'cflags!':
						[
							# Error in ../../thirdparty/libsqlite/include/qry_dat.h
							'-Werror=return-type',
						],
					},
				],
				[
					'OS == "mac" or OS == "ios"',
					{
						'xcode_settings':
						{
							'WARNING_CFLAGS!':
							[
								# Error in ../../thirdparty/libsqlite/include/qry_dat.h
								'-Werror=return-type',
								'-Werror=switch',
							],
						},
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
		},
		{
			'target_name': 'dbsqlite-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-dbsqlite',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_sqlite',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(dbsqlite_sources)',
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
				'server_mode': 1,
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
	
						'cflags!':
						[
							# Error in ../../thirdparty/libsqlite/include/qry_dat.h
							'-Werror=return-type',
						],
					},
				],
				[
					'OS == "mac"',
					{
						'xcode_settings':
						{
							'WARNING_CFLAGS!':
							[
								# Error in ../../thirdparty/libsqlite/include/qry_dat.h
								'-Werror=return-type',
								'-Werror=switch',
							],
						},
					},
				],
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
		{
			'target_name': 'external-revdb',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revdb',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../libexternal/libexternal.gyp:libExternal',
				'../libexternal/libexternal.gyp:libExternal-symbol-exports',
			],
			
			'defines':
			[
				'REVDB_BUILD',
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
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
				},
			},
			
			'conditions':
			[
				[
					'OS == "android"',
					{
						'product_name': 'RevDb',
						'product_extension': '',
					},
				],
				[
					'OS == "mac" or OS == "ios"',
					{
						'sources!': 
						[
							'src/unxsupport.cpp',
						],
					},
				],
				[
					'OS != "ios"',
					{
						'sources!':
						[
							'src/iossupport.cpp',
						],
					},
				],
			],
		},
		{
			'target_name': 'external-revdb-server',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'server-revdb',
			
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
			
			'defines':
			[
				'REVDB_BUILD',
			],

			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'<@(revdb_sources)',
			],
			
			'sources!':
			[
				'src/iossupport.cpp',
			],
			'conditions':
			[
				[
					'OS == "linux"',
					{
						'libraries':
						[
							'-Wl,-Bstatic',
							'-lstdc++',
							'-Wl,-Bdynamic',
						],
					},
				],
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(lib_suffix)' ],
				},
			},
		},
	],
}
