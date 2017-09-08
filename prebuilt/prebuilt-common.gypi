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
						'prebuilt_icu_bin_dir': 'bin/mac',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				'toolset_os == "ios"',
				{
					'variables':
					{
						'prebuilt_icu_bin_dir': 'bin/ios/$(SDK_NAME)',
						'prebuilt_icu_share_dir': 'share',
					}
				},
			],
			[
				'toolset_os == "linux"',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_icu_bin_dir': 'bin/linux/>(toolset_arch)',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				'toolset_os == "android"',
				{
					'variables':
					{
						# Gyp doesn't seem to handle non-absolute paths here properly...
						'prebuilt_icu_bin_dir': 'bin/android/<(target_arch)',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
			[
				'toolset_os == "win"',
				{
					'variables':
					{
						# Hack required due to GYP failure / refusal to treat this as a path
						'prebuilt_icu_bin_dir': '$(SolutionDir)../../prebuilt/unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/bin',
						'prebuilt_icu_share_dir': 'unpacked/icu/<(uniform_arch)-win32-$(PlatformToolset)_static_$(ConfigurationName)/share',
					},
				},
			],
			[
				'toolset_os == "emscripten"',
				{
					'variables':
					{
						'prebuilt_icu_bin_dir': 'bin/emscripten/js',
						'prebuilt_icu_share_dir': 'share',
					},
				},
			],
		],
	}
}
