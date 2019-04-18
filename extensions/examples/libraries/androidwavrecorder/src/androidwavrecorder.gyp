{

    'includes':
    [
        '../../../../../common.gypi',
    ],
    
    'targets':
    [
        {
            'target_name': 'androidwavrecorder',
            'type': 'none',
 
             'conditions':
            [
            	[
            		'OS == "android"',
            		{           
						'dependencies':
						[
							'androidwavrecorder-build',
						],
					},
				],
			],
        },
        {
            'target_name': 'androidwavrecorder-compile',
			'type': 'none',
			
			'sources':
			[
				'com/livecode/library/androidwavrecorder/ExtAudioRecorder.java',
			],  
			
            'conditions':
            [
            	[
            		'OS == "android"',
            		{
            			# Include the rules for compiling Java
						'includes':
						[
							'../../../../../config/java.gypi',
						],
						
						'variables':
						{
							'java_classes_dir_name': 'classes_androidwavrecorder',
						},						
            		}
            	],   
            ],	
        },		      
        {
            'target_name': 'androidwavrecorder-jar',
			'type': 'none',
			
            'conditions':
            [
            	[
            		'OS == "android"',
            		{			
						'dependencies':
						[
							'androidwavrecorder-compile',
						],			
					
						'sources':
						[
							'com/livecode/library/androidwavrecorder/ExtAudioRecorder.java',
						], 
								
						'actions':
						[
							{
								'action_name': 'jar',
								'message': 'JAR',

								'inputs':
								[
									# Depend on the Java source files directly to ensure correct updates
									'<@(_sources)',
								],

								'outputs':
								[
									'<(PRODUCT_DIR)/AndroidWavRecorder.jar',
								],

								'action':
								[
									'<(jar_path)',
									'cf',
									'<@(_outputs)',
									'-C', '<(PRODUCT_DIR)/classes_androidwavrecorder',
									'.',
								],
							},
						],    
					},
				],    
			]	    
        },
        {
        	'target_name': 'androidwavrecorder-build',
			'type': 'none',
			
            'conditions':
            [
            	[
            		'OS == "android"',
            		{
						'dependencies':
						[
							'androidwavrecorder-jar',
						],  
			
						'copies':
						[
							{
								'destination': '<(PRODUCT_DIR)/packaged_extensions/com.livecode.library.androidwavrecorder/code/jvm-android/',
								'files':
								[
									'<(PRODUCT_DIR)/AndroidWavRecorder.jar',
								],
							},
						],
					},
				],
			],
        },
    ],
}
