# Export snapshot with metadata
An optional `with metadata <metadata array>` clause has been added to the `export snapshot` command. Currently the only metadata key that is implemented is `density` which can be used to include pixel density metadata in pixels per inch.

For example:

    put 144 into theMetadataA["density"]
	export snapshot of group 1 at size the width of group 1 * 2, the height of group 1 * 2 with metadata theMetadataA



