script "CoreNetwork"
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

on TestSetup
   if the platform is "HTML5" then
      return "SKIP DNS and socket APIs unavailable on HTML5"
   end if
end TestSetup

on TestNetwork2
TestAssert "test", the dnsservers is not empty

repeat for each line tServer in the dnsservers
	set the itemdelimiter to "."
	TestAssert "test", the number of items in tServer is 4
	repeat for each item tComponent in tServer
		TestAssert "test", tComponent is an integer
		TestAssert "test", tComponent >= 0 and tComponent <= 255
	end repeat
end repeat

end TestNetwork2
on TestNetwork4

TestAssert "test", hostnametoaddress("google-public-dns-a.google.com") is "8.8.8.8"

end TestNetwork4
on TestNetwork13

put "www.livecode.com:80" into tSock
open socket tSock
TestAssert "test", tSock is among the lines of the openSockets
close socket tSock

end TestNetwork13

on TestPeerAddress
put "www.livecode.com:80" into tSock
open socket tSock 

local tTimeOut, tTime
put 100 into tTimeout
put the millisecs into tTime
repeat while the millisecs - tTime < tTimeOut
	get the peeraddress of tSock
	if it is not empty then
		TestAssert "peeraddress", it is among the lines of hostnametoaddress(tSock)
		close socket tSock
		exit TestPeerAddress
	end if
end repeat

TestSkip "peeraddress", "connection timed out"
close socket tSock
end TestPeerAddress