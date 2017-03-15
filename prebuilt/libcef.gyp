{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'libcef',
			'type': 'none',
			
			'dependencies':
			[
				'fetch.gyp:fetch',
			],
			
			'direct_dependent_settings':
			{
				'defines':
				[
					'USING_CEF_SHARED=1',
				],
			},
						
			'conditions':
			[
			    [
                    'OS == "win" or OS == "linux"',
                    {
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/CEF/locales',
                                'files':
                                [
                                    'lib/<(OS)/<(target_arch)/CEF/locales/am.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ar.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/bg.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/bn.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ca.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/cs.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/de.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/el.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/en-GB.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/en-US.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/es.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/es-419.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/et.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/fa.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/fi.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/fil.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/fr.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/gu.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/he.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/hi.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/hr.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/hu.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/id.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/it.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ja.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/kn.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ko.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/lt.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/lv.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ml.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/mr.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ms.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/nb.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/nl.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/pl.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/pt-BR.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/pt-PT.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ro.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ru.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/sk.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/sl.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/sr.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/sv.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/sw.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/ta.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/te.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/th.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/tr.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/uk.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/vi.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/zh-CN.pak',
                                    'lib/<(OS)/<(target_arch)/CEF/locales/zh-TW.pak',
                                ],
                            },
                        ],
                    },
                ],
                
                [
				    'OS == "win"',
				    {
					    'copies':
					    [
						    {
							    'destination':'<(PRODUCT_DIR)/CEF/',
							    'files':
							    [
								    'lib/win32/<(target_arch)/CEF/cef.pak',
								    'lib/win32/<(target_arch)/CEF/cef_100_percent.pak',
								    'lib/win32/<(target_arch)/CEF/cef_200_percent.pak',
								    'lib/win32/<(target_arch)/CEF/cef_extensions.pak',
								    'lib/win32/<(target_arch)/CEF/d3dcompiler_43.dll',
								    'lib/win32/<(target_arch)/CEF/d3dcompiler_47.dll',
								    'lib/win32/<(target_arch)/CEF/devtools_resources.pak',
								    'lib/win32/<(target_arch)/CEF/icudtl.dat',
								    'lib/win32/<(target_arch)/CEF/libcef.dll',
								    'lib/win32/<(target_arch)/CEF/libEGL.dll',
								    'lib/win32/<(target_arch)/CEF/libGLESv2.dll',
								    'lib/win32/<(target_arch)/CEF/natives_blob.bin',
								    'lib/win32/<(target_arch)/CEF/snapshot_blob.bin',
								    'lib/win32/<(target_arch)/CEF/widevinecdmadapter.dll',
							    ],
						    },
					    ],

					    'all_dependent_settings':
					    {
						    'variables':
						    {
							    # Gyp will only use a recursive xcopy if the path ends with '/'
							    'dist_aux_files': [ 'lib/win32/<(target_arch)/CEF/', ],
						    },
					    },
				    },
				],
				
				[
				    'OS == "linux"',
                    {
                        'copies':
                        [
                            {
                                'destination': '<(PRODUCT_DIR)/CEF/',
                                'files':
                                [
                                    'lib/linux/<(target_arch)/CEF/cef_100_percent.pak',
                                    'lib/linux/<(target_arch)/CEF/cef_200_percent.pak',
                                    'lib/linux/<(target_arch)/CEF/cef_extensions.pak',
                                    'lib/linux/<(target_arch)/CEF/cef.pak',
                                    'lib/linux/<(target_arch)/CEF/devtools_resources.pak',
                                    'lib/linux/<(target_arch)/CEF/libcef.so',
                                ],
                            },
                            {
                                'destination': '<(PRODUCT_DIR)/',
                                'files':
                                [
                                    'lib/linux/<(target_arch)/CEF/icudtl.dat',

                                    'lib/linux/<(target_arch)/CEF/natives_blob.bin',
                                    'lib/linux/<(target_arch)/CEF/snapshot_blob.bin',
                                ],
                            },
                            {
                                'destination': '<(PRODUCT_DIR)/CEF/',
                                'files':
                                [
                                    'lib/linux/<(target_arch)/CEF/icudtl.dat',
                                    'lib/linux/<(target_arch)/CEF/natives_blob.bin',
                                    'lib/linux/<(target_arch)/CEF/snapshot_blob.bin',
                                ],
                            },
                        ],
                        
					    'all_dependent_settings':
					    {
						    'variables':
						    {
							    'dist_aux_files': [ 'lib/linux/<(target_arch)/CEF/', ],
						    },
					    },
				    },
				],
			],
		},
	],
}
