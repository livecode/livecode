{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libExternal',
			'type': 'static_library',

			'toolsets': ['host', 'target'],

			'variables':
			{
				'library_for_module': 1,
			},

			'dependencies':
			[
				'../libcore/libcore.gyp:libCore',
			],

			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'include/revolution/external.h',
				'include/revolution/support.h',
				
				'src/external.c',
				'src/osxsupport.cpp',
				'src/unxsupport.cpp',
				'src/w32support.cpp',
			],
			
			'direct_dependent_settings':
			{
				'include_dirs':
				[
					'include',
				],
			},
			
			'conditions':
			[
				[
					'OS == "mac" or OS == "win"',
					{
						'sources!':
						[
							'src/unxsupport.cpp',
						],
					},
				],
			],
			
			'link_settings':
			{
				'conditions':
				[
					[
						'OS == "mac"',
						{
							'libraries':
							[
								'$(SDKROOT)/System/Library/Frameworks/CoreServices.framework',
							],
						},
					],
				],
			}
		},
		{
			'target_name': 'libExternal-symbol-exports',
			'type': 'none',
			
			'direct_dependent_settings':
			{				
				'variables':
				{
					'ios_external_symbols': 
					[ 
						'_getXtable', 
						'_setExternalInterfaceVersion', 
						'_configureSecurity',
					],
				},
			},
		},
	],
}
