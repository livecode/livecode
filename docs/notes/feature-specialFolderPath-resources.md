# Relocation of resources for Mac standalone applications

As stated in their [Technical Note 2206](https://developer.apple.com/library/mac/technotes/tn2206/_index.html#//apple_ref/doc/uid/DTS40007919-CH1-TNTAG205), Apple does not allow application to have unsigned code in the MacOS folder.

To comply with this new rule, every non-executable file is now copied in the folder `<App>.app/Contents/Resources/_MacOS`.

As for the executable files, they are still copied alongside the engine, and located in `specialFolderPath("engine")`.

This change does not impact any of the engine functions, which automatically redirect any path targeting the former location of the resource files.

However, the same does not apply to the externals, as they cannot determine if a path belongs to the app bundle or not; that affects also LiveCode external, such as revDatabase, this relocation breaks any script calling externals with a resource file.

In order to help with this overcoming this issue, the option **Resources** has been added for *specialFolderPath* on Desktop platforms, in LiveCode 6.7.2.

That will allow you to get the right path to the resources, being in development or standalone mode.

For instance, a code which used to be

		// Get the place Resources folder
		put the filename of this stack into tDatabasePath
		delete the last item of tDatabasePath
		// Append the SQLite file
		put "/database/db.sqlite" after tDatabasePath
		put revOpenDatabase("sqlite",tDatabasePath) into tDatabaseID
	
can be updated to

		put specialFolderPath("resources") into tDbPath
		// Append the SQLite file
		put "/database/db.sqlite" after tDatabasePath
		put revOpenDatabase("sqlite",tDatabasePath) into tDatabaseID
