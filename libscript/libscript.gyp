{
	'includes':
	[
		'../common.gypi',
	],
	
	'variables':
	{
		'libscript_public_headers':
		[
			'include/script.h',
			'include/script-auto.h',
		],
		
		'libscript_private_headers':
		[
			'src/script-private.h',
		],
		
		'libscript_sources':
		[
			'src/script-builder.cpp',
			'src/script-instance.cpp',
			'src/script-module.cpp',
			'src/script-object.cpp',
			'src/script-package.cpp',
		],
		
		'stdscript_sources':
		[
			'src/module-arithmetic.cpp',
			'src/module-array.cpp',
			'src/module-binary.cpp',
			'src/module-bitwise.cpp',
			'src/module-byte.cpp',
			'src/module-char.cpp',
			'src/module-codeunit.cpp',
			'src/module-date.cpp',
			'src/module-encoding.cpp',
			'src/module-file.cpp',
			'src/module-foreign.cpp',
			'src/module-list.cpp',
			'src/module-logic.cpp',
			'src/module-map.cpp',
			'src/module-math_foundation.cpp',
			'src/module-math.cpp',
			'src/module-sort.cpp',
			'src/module-stream.cpp',
			'src/module-string.cpp',
			'src/module-system.cpp',
			'src/module-type_convert.cpp',
			'src/module-type.cpp',
			'src/module-url.cpp',
		],
	},
	
	'targets':
	[
		{
			'target_name': 'libScript',
			'type': 'static_library',
			
			'toolsets': ['host','target'],
			
			'dependencies':
			[
				'../libfoundation/libfoundation.gyp:libFoundation',
				'../thirdparty/libffi/libffi.gyp:libffi',
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
		},
	],
}
