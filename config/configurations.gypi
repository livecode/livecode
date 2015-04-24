{
	'target_defaults':
	{
		'variables':
		{
			'server_mode': 0,		# Target being built for a server deployment
			'library_for_module': 0,	# Static library will be included in a dynamic lib
			'commercial': 0,		# Commercial or community build
		},
		
		'configurations':
		{
			'Debug':
			{

			},
			
			'Release':
			{

			},
			
			# Used by the CI system. Intended to be the fastest to build
			'Fast':
			{
			
			},
		},
	},
}
