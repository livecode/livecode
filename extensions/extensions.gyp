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
				'modules/widget-utils/widget-utils.lcb',
				'modules/scriptitems/scriptitems.lcb',

				'libraries/androidbgaudio/androidbgaudio.lcb',
				'libraries/canvas/canvas.lcb',
				'libraries/iconsvg/iconsvg.lcb',
				'libraries/json/json.lcb',

				'widgets/androidbutton/androidbutton.lcb',
				'widgets/browser/browser.lcb',
				#’widgets/chart/chart.lcb',
				#'widgets/checkbox/checkbox.lcb',
				'widgets/clock/clock.lcb',
				'widgets/graph/graph.lcb',
				'widgets/header/header.lcb',
				'widgets/iconpicker/iconpicker.lcb',
				#’widgets/list/list.lcb',
				#’widgets/multilist/multilist.lcb',
				'widgets/navbar/navbar.lcb',
				'widgets/paletteactions/paletteactions.lcb',
				#’widgets/pinkcircle/pinkcircle.lcb',
				#'widgets/progressbar/progressbar.lcb',
				#'widgets/pushbutton/pushbutton.lcb',
				#'widgets/radiobutton/radiobutton.lcb',
				'widgets/segmented/segmented.lcb',
				#'widgets/selector/selector.lcb',
				'widgets/svgpath/svgpath.lcb',
				'widgets/switchbutton/switchbutton.lcb',
				'widgets/treeview/treeview.lcb',
				'widgets/colorswatch/colorswatch.lcb',
				'widgets/gradientrampeditor/gradientrampeditor.lcb',
				'widgets/tile/tile.lcb',
				'widgets/spinner/spinner.lcb',
			],

			'all_dependent_settings':
			{
				'variables':
				{
					'dist_aux_files':
					[
						# Gyp will only use a recursive xcopy on Windows if the path ends with '/'
						'<(PRODUCT_DIR)/packaged_extensions/',
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
