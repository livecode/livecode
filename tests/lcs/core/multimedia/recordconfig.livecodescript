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

private function HasQT
    if the platform is not "MacOS" and the platform is not "Win32" then
       return false
    end if
    
    if the qtversion is "0.0" then
       return false
    end if
    
    if the environment is "server" then
       return false
    end if
    
    return true
end HasQT

on TestRecordCompressionTypes
	if HasQT() then
       TestAssert "record compression types not empty", \
          the recordCompressionTypes is not empty
  	end if
  	
  	repeat for each line tCompressionType in the recordCompressionTypes
  		set the recordCompression to tCompressionType
  		TestAssert "set record compression to" && tCompressionType, \
  			the recordCompression is tCompressionType
  	end repeat
end TestRecordCompressionTypes

on TestRecordFormats
	if HasQT() then
	   local tExpectedFormats
	   put "aiff,wave,ulaw,movie" into tExpectedFormats
	   replace comma with return in tExpectedFormats
       TestAssert "QT record compression formats", \
          the recordFormats is tExpectedFormats
  	end if
  	
  	repeat for each line tFormat in the recordFormats
  		set the recordFormat to tFormat
  		TestAssert "set record format to" && tFormat, \
  			the recordFormat is tFormat
  	end repeat
end TestRecordFormats
