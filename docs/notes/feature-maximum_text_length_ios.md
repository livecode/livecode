# Maximum text length on iOS native input fields

It is now possible to set/get the maximum number of characters that can be entered into an ios native single-line field, using 

	mobileControlSet sFieldID, "maximumTextLength", sMaxLength
	put mobileControlGet(sFieldID, "maximumTextLength") --> returns sMaxLength
