{
	# Common gyp inclusions for LiveCode
	'includes':
	[
		'config/arch.gypi',
		'config/configurations.gypi',
		'config/crosscompile.gypi',
		'config/exclusions.gypi',
		'config/perl.gypi',
		'config/target_os.gypi',
		'config/thirdparty.gypi',
		'config/version.gypi',
		'config/yacc.gypi',
	],
	
	# generate platform ID once all config is included
	'variables':
	{
		'conditions':
		[
			[
				'(OS == "mac" or OS == "ios")',
				{
					'platform_id': 'universal-<(OS)-<(target_sdk)',
				},
			],
			[
				'OS == "win"',
				{
					'platform_id': '<(uniform_arch)-<(OS)-msvc<(msvs_compiler_version)_<(msvs_crt_mode)',
				},
			],
			[
				'OS != "win" and OS != "mac" and OS != "ios"',
				{
					'platform_id': '<(uniform_arch)-<(OS)',
				},
			],
		],
	},	
}
