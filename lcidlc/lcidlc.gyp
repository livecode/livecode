{
	# ----- lcidlc -----
	'includes':
	[
		'../common.gypi',
	],

	'targets':
	[
        {
			'target_name': 'lcidlc',
			'type': 'executable',

            'dependencies':
			[
				'../libfoundation/libfoundation.gyp:libFoundation',
				'encode_support',
			],

			'include_dirs':
			[
				'include',
				'src',
			],

			'sources':
			[
				'include/LiveCode.h',

                'src/Coder.h',
				'src/Coder.cpp',
				'src/CString.h',
				'src/CString.cpp',
				'src/EncodedSupportJava.c',
				'src/EncodedSupport.c',
				'src/Error.h',
				'src/Error.cpp',
				'src/Interface.h',
				'src/Interface.cpp',
				'src/InterfaceGenerate.cpp',
				'src/InterfacePrivate.h',
				'src/Main.cpp',
				'src/NativeType.h',
				'src/NativeType.cpp',
				'src/Parser.h',
				'src/Parser.cpp',
				'src/Position.h',
				'src/Position.cpp',
				'src/Scanner.h',
				'src/Scanner.cpp',
				'src/Value.h',
				'src/Value.cpp',

			],
		},
        {
            'target_name': 'encode_support',
            'type': 'none',

            'sources':
			[
				'src/Support.java',
				'src/Support.mm',
			],

            'actions':
			[
				{
					'action_name': 'Encode Support.mm',

					'inputs':
                    [
                        '../util/encode_source.pl',
                        'src/Support.mm',
                    ],
                    'outputs':
                    [
                        'src/EncodedSupport.c',
                    ],

                    'action':
					[
						'<@(perl)',
						'../util/encode_source.pl',
						'src/Support.mm',
						'src/EncodedSupport.c',
						'g_support_template',
					],
				},
                {
					'action_name': 'Encode Support.java',

					'inputs':
                    [
                        '../util/encode_source.pl',
                        'src/Support.java',
                    ],
                    'outputs':
                    [
                        'src/EncodedJavaSupport.c',
                    ],

                    'action':
					[
						'<@(perl)',
						'../util/encode_source.pl',
						'src/Support.java',
						'src/EncodedJavaSupport.c',
						'g_java_support_template',
					],
				},
			],
        },
	],
}
