script "CoreRecordConfig"
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

on TestRecordCompressionTypes
	if (the platform is "MacOS" or the platform is "Win32") then
		if the qtversion is not "0.0" then
      			TestAssert "record compression types not empty", the recordCompressionTypes is not empty
    		end if
  	end if
  	
  	repeat for each line tCompressionType in the recordCompressionTypes
  		set the recordCompression to tCompressionType
  		TestAssert "set record compression to" && tCompressionType, \
  			the recordCompression is tCompressionType
  	end repeat
end TestRecordCompressionTypes

on TestRecordFormats
	if (the platform is "MacOS" or the platform is "Win32") then
		if the qtversion is not "0.0" then
			local tExpectedFormats
			put "aiff,wave,ulaw,movie" into tExpectedFormats
			replace comma with return in tExpectedFormats
      			TestAssert "QT record compression formats", the recordFormats is tExpectedFormats
    		end if
  	end if
  	
  	repeat for each line tFormat in the recordFormats
  		set the recordFormat to tFormat
  		TestAssert "set record format to" && tFormat, \
  			the recordFormat is tFormat
  	end repeat
end TestRecordFormats
