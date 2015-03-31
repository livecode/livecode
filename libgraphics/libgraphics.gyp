{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libGraphics',
			'type': 'static_library',
			
			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
				'../thirdparty/libskia/libskia.gyp:libskia',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'src/graphics-internal.h',
				
				'src/blur.cpp',
				'src/cachetable.cpp',
				'src/context.cpp',
				'src/coretext.cpp',
				'src/image.cpp',
				'src/legacyblendmodes.cpp',
				'src/legacygradients.cpp',
				'src/lnxtext.cpp',
				'src/mblandroidtext.cpp',
				'src/mbliphonetext.mm',
				#'src/osxtext.cpp', # UNUSED?
				'src/path.cpp',
				'src/region.cpp',
				'src/spread.cpp',
				'src/utils.cpp',
				'src/w32text.cpp',
			],
			
			'conditions':
			[
				[
					'OS != "mac" and OS != "ios"',
					{
						'sources!':
						[
							'src/coretext.cpp',
						],
					},
				],
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
		},
	],
}
