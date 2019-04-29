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

        '<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedJavaSupport.c',
        '<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedSupport.c',
			],

			'conditions':
			[
				[
					# Don't compile on android, iOS or emscripten
					'OS == "android" or OS == "ios" or OS == "emscripten"',
					{
						'type': 'none',
					},
				],
			],

			'msvs_settings':
			{
				'VCLinkerTool':
				{
					'SubSystem': 1,	# /SUBSYSTEM:CONSOLE
				},
			},
		},
    {
      'target_name': 'encode_support',
      'type': 'none',

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
              '<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedSupport.c',
          ],

          'action':
					[
						'<@(perl)',
						'../util/encode_source.pl',
						'src/Support.mm',
						'<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedSupport.c',
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
              '<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedJavaSupport.c',
          ],

          'action':
					[
						'<@(perl)',
						'../util/encode_source.pl',
						'src/Support.java',
						'<(SHARED_INTERMEDIATE_DIR)/lcidlc/EncodedJavaSupport.c',
						'g_java_support_template',
					],
				},
			],
    },
	],
}
