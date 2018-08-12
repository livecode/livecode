{
	'variables':
	{
		'version_major': "<!(perl <(DEPTH)/util/decode_version.pl BUILD_MAJOR_VERSION <(DEPTH)/version)",
		'version_minor': "<!(perl <(DEPTH)/util/decode_version.pl BUILD_MINOR_VERSION <(DEPTH)/version)",
		'version_point': "<!(perl <(DEPTH)/util/decode_version.pl BUILD_POINT_VERSION <(DEPTH)/version)",
		'version_build': "<!(perl <(DEPTH)/util/decode_version.pl BUILD_REVISION <(DEPTH)/version)",
		'version_string': "<!(perl <(DEPTH)/util/decode_version.pl BUILD_SHORT_VERSION <(DEPTH)/version)",

		'conditions':
		[
			[
				'build_edition == "commercial"',
				{
					'git_revision': '<!(git -C <(DEPTH)/.. rev-parse HEAD)',
				},
				{
					'git_revision': '<!(git -C <(DEPTH) rev-parse HEAD)',
				},
			],
		],
	},
}
