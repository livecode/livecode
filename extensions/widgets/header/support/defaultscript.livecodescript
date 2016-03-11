script "com.livecode.widget.headerBar.__DefaultScript"
on mouseUp
	local tActionName
	put the mouseAction of me into tActionName
	
	if tActionName is not empty then
		-- Execute script triggered by the given action
	end if
end mouseUp