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
			'OS == "win"',
			{
				# MSVS generates the debug databases automatically; we just need to copy them
				'variables':
				{
					'variables':
					{
						'conditions':
						[
							[
								# If configuring from a Unix-like environment, use its perl
								# otherwise use the detected Windows perl.
								'unix_configure != 0',
								{
									'perl_command': [ 'perl' ],
								},
								{
									'perl_command': [ '<@(perl)' ],
								},
							],
						],
					},
					
					# Note the use of the magic '<|(...)' expansion to write the
					# list of files out to another file: this prevents the
					# shell from attempting to expand any $(...) expressions in
					# the file list.
					'command':
					[
						'<@(perl_command)',
						'>(DEPTH)/tools/windows_debug_syms.pl',
						'>|(>(DEPTH)/debug_syms_inputs.txt <@(debug_syms_inputs))',
					],
					
					'debug_syms_outputs':
					[
						'>!@(<@(command))',
					],
				},
				
				'actions':
				[
					{
						'action_name': 'windows_debug_syms',
						'message': 'Extracting debug symbols',
						
						'inputs': [ '>@(debug_syms_inputs)', ],
						'outputs': [ '>@(debug_syms_outputs)', ],
						
						# Dummy action
						# This action is needed so that dependencies work correctly
						'action':
						[
							'echo',
							'Nothing to be done',
						],
					},
				],
			},
		],
		[
			'OS == "emscripten"',
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
