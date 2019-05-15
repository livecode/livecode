{
	'rules':
	[
		{
			'rule_name': 'unzip zip',
			'extension': 'zip',

			'message': '  UNZIP ZIP <(RULE_INPUT_PATH)',

			'outputs':
			[
				'<(PRODUCT_DIR)/<(RULE_INPUT_ROOT)',
			],

			'action':
			[
				'unzip',
				'-o',
				'<(RULE_INPUT_PATH)',
				'-d',
				'<@(_outputs)',
			],
		},

		{
			'rule_name': 'unzip aar',
			'extension': 'aar',

			'message': '  UNZIP AAR <(RULE_INPUT_PATH)',

			'outputs':
			[
				'<(PRODUCT_DIR)/<(RULE_INPUT_ROOT)',
			],

			'action':
			[
				'unzip',
				'-o',
				'<(RULE_INPUT_PATH)',
				'-d',
				'<@(_outputs)',
			],
		},
	],
}
