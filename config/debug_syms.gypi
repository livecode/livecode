{
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
							'<(extract-debug-symbols_path)',
							'<(OS)',
							'>(debug_info_suffix)',
							'>@(debug_syms_inputs)',
						],
					},
				],
			}
		],
	],
}
