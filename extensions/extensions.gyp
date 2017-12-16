{
	'includes':
	[
		'../common.gypi',
	],

    'variables':
    {
        'conditions':
        [
            [
                'host_os == "linux"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community',
                },
            ],
            [
                'host_os == "mac"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community',
                },
            ],
            [
                'host_os == "win"',
                {
                    'engine': '<(PRODUCT_DIR)/server-community.exe',
                },
            ],
        ],
    },
    
	'targets':
	[
	    {
	        'target_name': 'extensions',
			'type': 'none',
			
			'dependencies':
			[
				'cross-platform-extension-dependencies',
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
			
			'conditions':
			[
			    [
                    'mobile == 0',
                    {
                        'dependencies':
                        [
							'lcs-extensions',
                            'lcb-extensions',
                        ],	 
                    },
                ],
			],
	    },	    
		{
			'target_name': 'lcs-extensions',
			'type': 'none',
			
			'sources':
			[
				'script-libraries/oauth2/oauth2.livecodescript',
				'script-libraries/getopt/getopt.livecodescript',
				'script-libraries/mime/mime.livecodescript',
				'script-libraries/drawing/drawing.livecodescript',
				'script-libraries/dropbox/dropbox.livecodescript',
				'script-libraries/diff/diff.livecodescript',
				'script-libraries/messageauthentication/messageauthentication.livecodescript',
				'script-libraries/httpd/httpd.livecodescript',
				'script-libraries/qr/qr.livecodescript',
			],
			
			'dependencies':
		    [
        		# Requires a working LiveCode engine
		        '../engine/engine.gyp:server',
    		],
            
            'rules':
			[
				{
					'rule_name': 'build_script_extension',
					'extension': 'livecodescript',
					'message': 'Building script extension <(RULE_INPUT_NAME)',
										
					'outputs':
					[
						'<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.<(RULE_INPUT_ROOT)/<(RULE_INPUT_ROOT).livecodescript',
					],
					          
					'action':
					[
						'<(engine)',
						'../util/package_extension.livecodescript',
						'dummy1',
						'dummy2',
						'dummy3',
						'../util/extract-docs.livecodescript',
						'../ide-support/revdocsparser.livecodescript',			
						'<(RULE_INPUT_DIRNAME)',
						'<(RULE_INPUT_ROOT).livecodescript',
						'<(PRODUCT_DIR)/packaged_extensions',
						'false',
					],
				},
			],
		},
		
		{
			'target_name': 'lcb-extensions',
			'type': 'none',

			'dependencies':
			[
				'../toolchain/lc-compile/lc-compile.gyp:lc-compile',
				'../engine/lcb-modules.gyp:engine_lcb_modules',
				'../engine/engine.gyp:server',
			],

			'sources':
			[
				'modules/widget-utils/widget-utils.lcb',
				'modules/scriptitems/scriptitems.lcb',

				'libraries/androidbgaudio/androidbgaudio.lcb',
				'libraries/androidaudiorecorder/androidaudiorecorder.lcb',
				'libraries/toast/toast.lcb',
				'libraries/canvas/canvas.lcb',
				'libraries/iconsvg/iconsvg.lcb',
				'libraries/json/json.lcb',
				'libraries/objectrepository/objectrepository.lcb',
				'libraries/ini/ini.lcb',

				'widgets/androidbutton/androidbutton.lcb',
				'widgets/androidfield/androidfield.lcb',
				'widgets/macbutton/macbutton.lcb',
				'widgets/mactextfield/mactextfield.lcb',
				'widgets/iosbutton/iosbutton.lcb',
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
						# hack because gyp wants an output
                        'notarealfile.txt',
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
						'<(engine)',
						'../util/package_extension.livecodescript',
						'../util/extract-docs.livecodescript',
						'../ide-support/revdocsparser.livecodescript',
						'<(PRODUCT_DIR)/packaged_extensions',
						'false',
						'>(lc-compile_host)',
						'<(PRODUCT_DIR)/modules/lci',
						'<@(_sources)',
					],
				},
			],
		},
		
		{
			'target_name': 'cross-platform-extension-dependencies',
			'type': 'none',

			'dependencies':
			[
				'libraries/ini/inih/inih.gyp:inih',
			],
		},
	],
}
