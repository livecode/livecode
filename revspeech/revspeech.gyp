{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revspeech',
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
				'src/osxspeech.h',
				'src/revspeech.h',
				'src/w32sapi4speech.h',
				'src/w32sapi5speech.h',
				'src/osxspeech.cpp',
				'src/revspeech.cpp',
				'src/w32sapi4speech.cpp',
				'src/w32sapi5speech.cpp',
				'src/w32speech.cpp',
			],
			
			'conditions':
			[
				[
					'OS != "mac" and OS != "win"',
					{
						'type': 'none',
					},
				],
				[
					'OS == "mac"',
					{
						'libraries':
						[
							'$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
						],
					},
				],
				[
					'OS == "win"',
					{
						'include_dirs':
						[
							'<(ms_speech_sdk4)/Include',
						],
						
						'libraries':
						[
							'-ladvapi32',
							'-lole32',
						],
					},
				],
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revspeech-Info.plist',
			},
		},
	],
}
