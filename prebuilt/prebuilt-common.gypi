{
	'target_defaults':
	{
		'target_conditions':
		[
			[
				'toolset_os == "mac"',
				{
					'variables':
					{
						'prebuilt_bin_dir': 'bin/mac',
						'prebuilt_share_dir': 'share/mac',
					},
				},
			],
			[
				'toolset_os == "ios"',
				{
					'variables':
					{
						'prebuilt_bin_dir': 'bin/ios/$(SDK_NAME)',
						'prebuilt_share_dir': 'share/ios/$(SDK_NAME)',
					}
				},
			],
			[
				'toolset_os == "linux"',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_bin_dir': 'bin/linux/>(toolset_arch)',
						'prebuilt_share_dir': 'share/linux/>(toolset_arch)',
					},
				},
			],
			[
				'toolset_os == "android"',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_bin_dir': 'bin/android/<(target_arch)',
						'prebuilt_share_dir': 'share/android/<(target_arch)',
					},
				},
			],
			[
				'toolset_os == "win"',
				{
					'variables':
					{
						'prebuilt_bin_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/bin',
						'prebuilt_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
					},
				},
			],
			[
				'toolset_os == "emscripten"',
				{
					'variables':
					{
						'prebuilt_bin_dir': 'bin/emscripten/js',
						'prebuilt_share_dir': 'share/emscripten/js',
					},
				},
			],
		],
	}
}
