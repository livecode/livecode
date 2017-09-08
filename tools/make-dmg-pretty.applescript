on run argv
	if the number of items of argv < 2 then
		display alert "Usage: make-dmg-pretty <volume name> <background image>"
		return "Error: invalid parameters"
	end if
	
	set dmgIn to item 1 of argv
	set backgroundImage to POSIX file (item 2 of argv)
	tell application "Finder"
		
		-- Delay to make sure the disk has had time to mount
		delay 10
		
		tell disk dmgIn
			open
			
			-- Path to this volume
			set diskPath to POSIX path of (it as alias)
			
			-- Remove any existing preferences
			do shell script "rm -f " & quoted form of (diskPath & "/.DS_Store")
			
			-- Remove any existing background folder
			do shell script "rm -rf " & quoted form of (diskPath & "/.background")
			
			-- Remove any existing Applications alias
			if exists item "Applications" then
				delete item "Applications"
			end if
			
			-- Create a new alias to the Applications folder
			make new alias at it to POSIX file "/Applications"
			
			-- Create a hidden directory for the folder background
			make new folder at it with properties {name:".background"}
			
			-- Copy the background image into place
			-- AppleScript seems to treat "." directories as neither files nor folders so the shell is needed here
			do shell script "cp " & quoted form of POSIX path of backgroundImage & " " & quoted form of (diskPath & "/.background/bg.tiff")
			
			-- Configure the Finder window's appearance
			set the current view of container window to icon view
			set toolbar visible of container window to false
			set statusbar visible of container window to false
			set the bounds of container window to {100, 100, 646, 515}
			set viewOptions to the icon view options of container window
			set arrangement of viewOptions to not arranged
			set icon size of viewOptions to 128
			set text size of viewOptions to 14
			set background picture of viewOptions to file "bg.tiff" of item ".background"
			
			-- Set the position of the icons
			set fileList to every item of it
			repeat with i in fileList
				-- We're slightly fuzzy here as the app bundle name varies with edition and version
				if the name of i is "Applications" then
					set the position of i to {410, 177}
				else if the name of i ends with ".app" then
					set the position of i to {137, 191}
				else
					set the position of i to {700, 700}
				end if
			end repeat
			
			-- Close and re-open the window to force refresh
			--close
			--open
			
			-- According to http://joemaller.com/659/setting-icon-position-and-window-size-on-disk-images/, Finder sometimes does not save the size and position of a window until it detects user interaction on either the window's resizing handle or its zoom button. Simulate two clicks on the zoom button to trigger the save.
			--set theWindow to container window
			--tell application "System Events" to tell process "Finder" to click button 2 of theWindow
			--tell application "System Events" to tell process "Finder" to click button 2 of theWindow
			
			-- Update everything after a delay to allow Finder to synchronise
			delay 10
			update without registering applications
			
			-- Eject the disk image
			eject
			
			-- Delay to allow unmounting
			delay 10
		end tell
	end tell
end run
