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
			'mac_bundle': 1,
			'product_prefix': '',
			'product_name': 'revvideograbber',

			'variables':
			{
				'enable_revvideograbber%': '1',
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
					'(OS != "mac" and OS != "win") or enable_revvideograbber == 0',
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
				],
				[
					'OS == "mac" and enable_revvideograbber != 0',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
							'$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
							'$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
							'$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
							'$(SDKROOT)/System/Library/Frameworks/Quartz.framework',
							'$(SDKROOT)/System/Library/Frameworks/QuickTime.framework',
						],
					},
				],
				[
					'OS == "win" and enable_revvideograbber != 0',
					{
						'include_dirs':
						[
							'<(quicktime_sdk)/CIncludes',
						],
						
						'library_dirs':
						[
							'<(quicktime_sdk)/Libraries',
						],
						
						'libraries':
						[
							'-lgdi32',
							'-luser32',
							'-ladvapi32',
							'-lole32',
							'-loleaut32',
							'-lstrmiids',
							'-lvfw32',
							'-lQTMLClient'
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revvideograbber-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revvideograbber.exports',
				
				# The QuickTime support we need was dropped after 10.6. Correspondingly, it doesn't
				# work when built for 64-bit either.
				'SDKROOT': 'macosx10.6',
				'ARCHS': 'i386',
				
				# Gyp adds "-x c++" to the build process, which is a problem as one of the .cpp files
				# contains some Objective-C code. Changing it to .mm would be the obvious solution, but
				# we also need to compile that file on Windows. Force using Objective-C++
				'OTHER_CPLUSPLUSFLAGS': '-x objective-c++',
			},
		},
	],
}
