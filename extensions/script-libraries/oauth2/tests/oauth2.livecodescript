script "TestOAuth2Lib"
/*
Copyright (C) 2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.
*/

on TestSetup
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/oauth2/oauth2.livecodescript" \
         into tLibFilename

   start using stack tLibFilename
end TestSetup

on TestPortRange
   OAuth2Set "port", -1
   TestAssert "negative port is invalid", the result is not empty
   
   OAuth2Set "port", 0
   TestAssert "zero port is invalid", the result is not empty
   
   OAuth2Set "port", 8080
   TestAssert "in range port is valid", the result is empty
   
   OAuth2Set "port", 65535
   TestAssert "end of range port is valid", the result is empty

   OAuth2Set "port", 65536
   TestAssert "out of range port is invalid", the result is not empty
end TestPortRange
