script "AndroidNativeFieldWidgetProperties"
/*
Copyright (C) 2017 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.scriptitems"
	TestLoadExtension "com.livecode.library.androidutils"	
	TestLoadExtension "com.livecode.widget.native.android.field"
end TestSetup

constant kProps = "text,fieldTextColor,textAlign,verticalTextAlign,autoCapitalizationType,autoCorrectionType,keyboardType,returnKeyType,contentType,dataDetectorTypes,multiline,editable,scrollingEnabled"

on TestProperties
	create widget "TestWidget" as "com.livecode.widget.native.android.field"
	repeat for each item tProp in kProps
		get the tProp of widget "TestWidget"
		set the tProp of widget "TestWidget" to it
	end repeat
end TestProperties
