{
	'variables':
	{
		'version_major': "<!(perl -n -e '/^BUILD_MAJOR_VERSION[[:blank:]]*=[[:blank:]]*(.*)$/ && print $1' <(DEPTH)/version)",
		'version_minor': "<!(perl -n -e '/^BUILD_MINOR_VERSION[[:blank:]]*=[[:blank:]]*(.*)$/ && print $1' <(DEPTH)/version)",
		'version_point': "<!(perl -n -e '/^BUILD_POINT_VERSION[[:blank:]]*=[[:blank:]]*(.*)$/ && print $1' <(DEPTH)/version)",
		'version_build': "<!(perl -n -e '/^BUILD_REVISION[[:blank:]]*=[[:blank:]]*(.*)$/ && print $1' <(DEPTH)/version)",
	
		'version_string': "<!(perl -n -e '/^BUILD_SHORT_VERSION[[:blank:]]*=[[:blank:]]*(.*)$/ && print $1' <(DEPTH)/version)",

		'git_revision': '<!(git rev-parse HEAD)',
	},
}
