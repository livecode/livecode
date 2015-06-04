{
	'variables':
	{
		'output_dir': '../linux-<(target_arch)-bin',
	},
	
	'target_defaults':
	{
		'includes':
		[
			'linux-settings.gypi',
		],
	},
}
