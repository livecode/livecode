on run argv
	if the number of items of argv is not 1 then
		return "Error: invalid parameters"
	end if
	
	set folderPath to POSIX file (item 1 of argv)
	
	tell application "Finder"
		tell folder folderPath
			-- Open a Finder window on the DMG
			open
			
			-- Bring that window to the front
			activate
		end tell
	end tell
end run
