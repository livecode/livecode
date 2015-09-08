{
	'conditions':
	[
		[
			'OS != "win" and OS != "emscripten"',
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
			'OS == "win" or OS == "emscripten"',
			{
				# Not yet implemented...
				'variables':
				{
					'debug_syms_outputs': [],
				},
			},
		],
	],
}
