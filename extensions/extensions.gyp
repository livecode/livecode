{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'extensions',
			'type': 'none',
			
			'dependencies':
			[
				'../toolchain/lc-compile/lc-compile.gyp:lc-compile',
				'../engine/lcb-modules.gyp:engine_lcb_modules',
			],
			
			'sources':
			[
				'libraries/canvas/canvas.lcb',
				'libraries/iconsvg/iconsvg.lcb',
				
				'widgets/button/button.lcb',
				#'widgets/button-popup/button-popup.lcb',
				'widgets/chart/chart.lcb',
				'widgets/checkbox/checkbox.lcb',
				'widgets/clock/clock.lcb',
				'widgets/graph/graph.lcb',
				'widgets/header/header.lcb',
				'widgets/iconpicker/iconpicker.lcb',
				'widgets/list/list.lcb',
				'widgets/multilist/multilist.lcb',
				'widgets/navbar/navbar.lcb',
				'widgets/paletteactions/paletteactions.lcb',
				'widgets/pinkcircle/pinkcircle.lcb',
				'widgets/progressbar/progressbar.lcb',
				'widgets/radiobutton/radiobutton.lcb',
				'widgets/segmented/segmented.lcb',
				#'widgets/segmented-popup/segmented-popup.lcb',
				'widgets/selector/selector.lcb',
                'widgets/simplecomposed/simplecomposed.lcb',
				#'widgets/svgicon/svgicon.lcb',
				'widgets/svgpath/svgpath.lcb',
				'widgets/switchbutton/switchbutton.lcb',
				'widgets/treeview/treeview.lcb',
			],
			
			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files':
					[
						'<(PRODUCT_DIR)/packaged_extensions',
					],
				},
			},
			
			'actions':
			[
				{
					'action_name': 'build_extensions',
					
					'inputs': 
					[
						'../util/build-extensions.sh',
						'<@(_sources)',
					],
					
					'outputs':
					[
						'<(PRODUCT_DIR)/packaged_extensions',
					],
					
					'message': 'Building extensions',
					
					'conditions':
					[
						[
							'OS == "win"',
							{
								'variables':
								{
									'build_command': [ '$(ProjectDir)../../../util/invoke-unix.bat', '$(ProjectDir)../../../util/build-extensions.sh' ],
								},
							},
							{
								'variables':
								{
									'build_command': [ '../util/build-extensions.sh' ],
								},
							},
						],
					],
					
					'action':
					[
						'<@(build_command)',
						'<(PRODUCT_DIR)/packaged_extensions',
						'<(PRODUCT_DIR)/modules/lci',
						'>(lc-compile_host)',
						'<@(_sources)',
					],
				},
			],
		},
	],
}
