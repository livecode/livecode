{
	'targets':
	[
		{
			'target_name': 'debug-symbols',
			'type': 'none',

			'dependencies':
			[
				'LiveCode-all',
			],

			'variables':
			{
				'debug_syms_inputs%': [ '<@(debug_syms_inputs)' ],
				'variables':
				{
					'debug_syms_inputs': [ '>@(dist_files)' ],
				},
			},
			
			'conditions':
			[
				[
					'OS != "win"',
					{
						'variables':
						{
							'debug_syms_outputs':
							[
								'>!@(["sh", "-c", "echo $@ | xargs -n1 | sed -e \\\"s/$/>(debug_info_suffix)/g\\\"", "echo", \'>@(debug_syms_inputs)\'])',
							],
					
							'extract-debug-symbols_path': '../tools/extract-debug-symbols.sh',
					
							# These tools are only used for Linux and Android targets
							'objcopy%': '',
							'objdump%': '',
							'strip%': '',
						},

						'actions':
						[
							{
								'action_name': 'extract-debug-symbols',
								'message': 'Extracting debug symbols',
			
								'inputs':
								[
									'>@(debug_syms_inputs)',
									'<(extract-debug-symbols_path)',
								],
			
								'outputs':
								[
									'>@(debug_syms_outputs)',
								],
			
								'action':
								[
									'env',
									'OBJCOPY=<(objcopy)',
									'OBJDUMP=<(objdump)',
									'STRIP=<(strip)',
									'<(extract-debug-symbols_path)',
									'<(OS)',
									'>(debug_info_suffix)',
									'>@(debug_syms_inputs)',
								],
							},
						],
					}
				],
				[
					'OS == "win"',
					{
						# Not yet implemented...
						'variables':
						{
							'debug_syms_outputs': [],
						},
					},
				],
			],


			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files': [ '<@(debug_syms_outputs)' ],
					'variables':
					{
						'debug_syms_inputs%': [ '<@(debug_syms_inputs)' ],
					},
				},
			},
		},
		{
			'target_name': 'binzip-copy',
			'type': 'none',
	
			'variables':
			{
				'dist_files': [],
				'dist_aux_files': [],
			},
	
			'dependencies':
			[
				'LiveCode-all',
				'debug-symbols',
			],
	
			'copies':
			[{
				'destination': '<(output_dir)',
				'files': [ '>@(dist_files)', '>@(dist_aux_files)', ],
			}],
		},
	],
}
