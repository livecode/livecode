{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'external-revvideograbber',
			'type': 'loadable_module',
			'product_prefix': '',
			'product_name': 'revvideograbber',

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
				'src/dsvideograbber.h',
				'src/mcivideograbber.h',
				'src/qedit.h',
				'src/qtvideograbber.h',
				'src/qtxvideograbber.h',
				'src/revcapture.h',
				'src/revvideograbber.h',
				'src/videograbber.h',
				
				'src/dsDlgFunc.cpp',
				'src/dsvideograbber.cpp',
				'src/mcivideograbber.cpp',
				'src/qtvideograbber.cpp',
				'src/qtxcapture.mm',
				'src/qtxvideograbber.mm',
				'src/revcapture.mm',
				'src/revvideograbber.cpp',
				'src/rrecapture.m',
			],
			
			'conditions':
			[
				[
					'OS != "win"',
					{
						'type': 'none',
						'mac_bundle': '0',
					},
					{
						'all_dependent_settings':
						{
							'variables':
							{
								'dist_files': [ '<(PRODUCT_DIR)/<(_product_name)>(ext_bundle_suffix)' ],
							},
						},
					},
				],
				[
					'OS != "win"',
					{
						'sources/':
						[
							['exclude', '^src/(ds|mci).*\\.cpp$'],
						],
					},
					{
						'sources/':
						[
							['exclude', '^src/(qt).*\\.cpp$'],
						],
					},
				],
				[
					'OS == "win"',
					{
						'libraries':
						[
							'-lgdi32',
							'-luser32',
							'-ladvapi32',
							'-lole32',
							'-loleaut32',
							'-lstrmiids',
							'-lvfw32',
						],
					},
				],
			],
		},
	],
}
