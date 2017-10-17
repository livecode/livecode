script "CoreMultimedia"
/*
Copyright (C) 2015 LiveCode Ltd.

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


on TestMultimedia1


if the platform is not "win32" then
	TestAssert "test", mcisendstring("") is "not supported"
else
	local tQuantity
	put mcisendstring("sysinfo all quantity") into tQuantity
	TestAssert "test", tQuantity is not empty
	TestAssert "test", tQuantity is an integer
	
	get mcisendstring("invalid command")
	TestAssert "test", the result is not empty
end if

end TestMultimedia1

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

on TestMultimedia2

if HasQT() then
  TestAssert "test", the qteffects is not empty
else
  TestAssert "test", the qteffects is empty
end if

end TestMultimedia2
on TestMultimedia3


local tVersion
put the qtversion into tVersion

set the itemdelimiter to "."
repeat for each item tComponent in tVersion
   TestAssert "test", tComponent is an integer
end repeat

end TestMultimedia3
on TestMultimedia4

if the platform is not "linux" then
TestAssert "test", the recordloudness is a number
else
TestAssert "test", true
end if

end TestMultimedia4
