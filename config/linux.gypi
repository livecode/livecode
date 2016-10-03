{
	'variables':
	{
		'output_dir': '../linux-<(target_arch)-bin',
		
		# Capture the values of some build tool environment vars
		'objcopy': '<!(echo ${OBJCOPY:-objcopy})',
		'objdump': '<!(echo ${OBJDUMP:-objdump})',
		'strip':   '<!(echo ${STRIP:-strip})',
		'javahome': '<!(echo ${JAVA_HOME:-javahome})',
	},
	
	'target_defaults':
	{
		'variables':
		{
			'silence_warnings': 0,
		},

		'includes':
		[
			'linux-settings.gypi',
		],
	},
}
