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
			# TODO add windows msvc compiler and crt mode to platform id
			[
				'(OS == "mac" or OS == "ios")',
				{
					'platform_id': 'universal-<(OS)-<(target_sdk)',
				},
				'(OS == "win")',
				{
				'platform_id': '<(uniform_arch)-win32',
				},
				{
					'platform_id': '<(uniform_arch)-<(OS)',
				},
			],
		],
	},	
}
