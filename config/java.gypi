{
	'variables':
	{
		'javac_wrapper_path': '../tools/javac_wrapper.sh',
	},
	
	'rules':
	[
			
		{
			'rule_name': 'aidl_interface_gen',
			'extension': 'aidl',

			'message': '  AIDL <(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).java',
			'process_outputs_as_sources': 1,

			'inputs':
			[
				'<(aidl_framework_path)',
			],

			'outputs':
			[
				'<(INTERMEDIATE_DIR)/<(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).java',
			],

			'action':
			[
				'<(aidl_path)',
				'-Isrc/java',
				'-p' '<@(_inputs)',
				'-o' '<(INTERMEDIATE_DIR)/src/java',
				'<(RULE_INPUT_PATH)',
			],
		},

		{
			'rule_name': 'javac',
			'extension': 'java',

			'message': '  JAVAC <(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).class',

			'variables':
			{
				'java_classpath_param': ">!(['<(pathify_path)', '<(java_classpath)', '>@(java_extra_classpath)', '<(PRODUCT_DIR)/>(java_classes_dir_name)'])",
				
				'java_extra_classpath%': '>(java_extra_classpath)',
				'variables':
				{
					'java_extra_classpath%': '',
					'pathify_path': '../tools/pathify.sh',
				},
				
				'java_source_path': '../engine/src/java',
			},

			'outputs':
			[
				# Java writes the output file based on the class name.
				# Use some Make nastiness to correct the output name
				'<(PRODUCT_DIR)/>(java_classes_dir_name)/$(subst\t../livecode/engine/,,$(subst\t<(INTERMEDIATE_DIR)/,,$(subst\tsrc/java/,,<(RULE_INPUT_DIRNAME))))/<(RULE_INPUT_ROOT).class',
			],

			'action':
			[
				'<(javac_wrapper_path)',
				'<(javac_path)',
				'1.7',
				'-d', '<(PRODUCT_DIR)/>(java_classes_dir_name)',
				'-implicit:none',
				'-classpath', '>(java_classpath_param)',
				'-sourcepath', '<(java_source_path):<(INTERMEDIATE_DIR)/src/java',
				'-encoding utf8',
				'<(RULE_INPUT_PATH)',
			],
		},
	],
}
