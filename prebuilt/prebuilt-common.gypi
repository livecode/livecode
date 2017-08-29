{
	'target_defaults':
	{
		'variables':
		{
			'target_conditions':
			[
				[
					'OS == "mac"',
					{
						'prebuilt_bin_dir': 'bin/mac',
						'prebuilt_share_dir': 'share/mac',
					},
				],
				[
					'OS == "ios"',
					{
						'prebuilt_bin_dir': 'bin/ios/$(SDK_NAME)',
						'prebuilt_share_dir': 'share/ios/$(SDK_NAME)',
					},
				],
				[
					'OS == "linux"',
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_bin_dir': 'bin/linux/>(toolset_arch)',
						'prebuilt_share_dir': 'share/linux/>(toolset_arch)',
					},
				],
				[
					'OS == "android"',
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_bin_dir': 'bin/android/<(target_arch)',
						'prebuilt_share_dir': 'share/android/<(target_arch)',
					},
				],
				[
					'OS == "win"',
					{
						'prebuilt_bin_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/bin',
						'prebuilt_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
					},
				],
				[
					'OS == "emscripten"',
					{
						'prebuilt_bin_dir': 'bin/emscripten/js',
						'prebuilt_share_dir': 'share/emscripten/js',
					},
				],
				
			],
		},
	}
}
