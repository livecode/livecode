{
	'includes':
	[
		'../common.gypi',
		'stdscript-sources.gypi',
	],
	
	'variables':
	{
		'libscript_public_headers':
		[
			'include/libscript/script.h',
			'include/libscript/script-auto.h',
		],
		
		'libscript_private_headers':
		[
			'src/script-private.h',
			'src/script-bytecode.hpp',
			'src/script-execute.hpp',
			'src/script-validate.hpp',
		],
		
		'libscript_sources':
		[
			'src/script-builder.cpp',
			'src/script-instance.cpp',
			'src/script-module.cpp',
			'src/script-object.cpp',
			'src/script-package.cpp',
			'src/script-execute.cpp',
			'src/script-execute-objc.mm',
			'src/script-error.cpp',
		],
	},
	
	'targets':
	[
		{
			'target_name': 'libScript',
			'type': 'static_library',
			
			'toolsets': ['host','target'],
			
			'product_prefix': '',
			'product_name': 'libScript',
			
			'dependencies':
			[
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../prebuilt/thirdparty.gyp:thirdparty_prebuilt_ffi',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'<@(libscript_public_headers)',
				'<@(libscript_private_headers)',
				'<@(libscript_sources)',
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
					'OS != "mac" and OS != "ios"',
					{
						'sources!':
						[
							'src/script-execute-objc.mm',
						],
					},
				],
				[
					'OS == "linux" or OS == "android"',
					{
						'link_settings':
						{
							'libraries':
							[
								'-ldl',
							],
						},	
					},
				],
			],
			
			'target_conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'libScript->(_toolset)',
					},
				],
			],
		},
		{
			'target_name': 'stdscript',
			'type': 'static_library',
			
			'toolsets': ['host','target'],
			
			'product_prefix': '',
			'product_name': 'stdscript',
			
			'dependencies':
			[
				'libScript',
				'../libfoundation/libfoundation.gyp:libFoundation',
			],
			
			'include_dirs':
			[
				'include',
				'src',
			],
			
			'sources':
			[
				'<@(stdscript_sources)',
			],
			
			'target_conditions':
			[
				[
					'_toolset != "target"',
					{
						'product_name': 'stdscript->(_toolset)',
					},
				],
			],
		},
	],
}
