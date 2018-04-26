script "TimeZoneLibrary"
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
    TestSkipIfNot "platform", "macos,linux"
	TestLoadExtension "com.livecode.library.timezone"
end TestSetup

on TestTimezoneRoundTrip
   local tSeconds, tRoundTrip
   put the seconds into tSeconds
   repeat for each line tZone in TimeZones()
      try
         put ToUniversalTime(FromUniversalTime(tSeconds, tZone), tZone) into tRoundTrip
         TestAssert tZone & "timezone round trip", tRoundTrip is tSeconds
      catch tError
         TestAssert tZone & "timezone round trip", false
      end try
   end repeat
end TestTimezoneRoundTrip
