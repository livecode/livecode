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

on TestOpenSocketFrom
   open socket from ":8000" to "livecode.com"
   TestAssert "open socket from", "livecode.com" is among the lines of the openSockets
   close socket "livecode.com"
end TestOpenSocketFrom

on TestOpenSocketFromWithLSOF
   if the platform is "MacOS" or the platform contains "linux" then
      open socket from ":8001" to "livecode.com"
      get shell("lsof -i4TCP:8001")
      TestAssert "open socket from testing with lsof", it contains "LiveCode"
      close socket "livecode.com"
   end if
end TestOpenSocketFromWithLSOF

on TestOpenSocketFromMultipleHostsSamePort
   open socket from ":8002" to "livecode.com"
   open socket from ":8002" to "google.com"
   TestAssert "open socket from with multiple hosts on same local port", "livecode.com" is among the lines of the openSockets
   TestAssert "open socket from with multiple hosts on same local port", "google.com" is among the lines of the openSockets
   close socket "livecode.com"
   close socket "google.com"
end TestOpenSocketFromMultipleHostsSamePort

on TestOpenSocketFromSameHostDifferentPort
   open socket from ":8003" to "livecode.com|1"
   open socket from ":8004" to "livecode.com|2"
   TestAssert "open socket from with same host on different local port", "livecode.com|1" is among the lines of the openSockets
   TestAssert "open socket from with same host on different local port", "livecode.com|2" is among the lines of the openSockets
   close socket "livecode.com|1"
   close socket "livecode.com|2"
end TestOpenSocketFromSameHostDifferentPort

on TestOpenSocketFromWithAddress
   local tLocalAddress
   repeat for each line tInterface in the networkInterfaces
      if tInterface is not "127.0.0.1" then
         put tInterface into tLocalAddress
         exit repeat
      end if
   end repeat

   if tLocalAddress is not empty then
      open socket from tLocalAddress to "livecode.com"
      TestAssert "open socket from with local address", "livecode.com" is among the lines of the openSockets
      close socket "livecode.com"
   end if
end TestOpenSocketFromWithAddress

on TestOpenSocketFromWithAddressAndPort
   local tLocalAddress
   repeat for each line tInterface in the networkInterfaces
      if tInterface is not "127.0.0.1" then
         put tInterface into tLocalAddress
         exit repeat
      end if
   end repeat

   if tLocalAddress is not empty then
      open socket from tLocalAddress & ":8005" to "livecode.com"
      TestAssert "open socket from with local address and port", "livecode.com" is among the lines of the openSockets
      close socket "livecode.com"
   end if
end TestOpenSocketFromWithAddressAndPort

on TestOpenSocketFromMultipleHostsSameAddress
   local tLocalAddress
   repeat for each line tInterface in the networkInterfaces
      if tInterface is not "127.0.0.1" then
         put tInterface into tLocalAddress
         exit repeat
      end if
   end repeat

   if tLocalAddress is not empty then
      open socket from tLocalAddress & ":8006" to "livecode.com"
      open socket from tLocalAddress & ":8006" to "google.com"
      TestAssert "open socket from with multiple hosts using same local address", "livecode.com" is among the lines of the openSockets
      TestAssert "open socket from with multiple hosts using same local address", "google.com" is among the lines of the openSockets
      close socket "livecode.com"
      close socket "google.com"
   end if
end TestOpenSocketFromMultipleHostsSameAddress

on TestOpenSocketFromSameHostDifferentAddress
   local tLocalAddresses
   put the networkInterfaces into tLocalAddresses
   filter tLocalAddresses without "127.0.0.1"

   if the number of lines in tLocalAddresses > 1 then
      local tSocketCount
      put 1 into tSocketCount
      repeat for each line tLocalAddress in tLocalAddresses
         open socket from tLocalAddress & ":8007" to "livecode.com|" & tSocketCount
         TestAssert "open socket from with same host on different local address", "livecode.com|" & tSocketCount is among the lines of the openSockets
         add 1 to tSocketCount
      end repeat
      repeat with tSocketCount = 1 to the number of lines in tLocalAddresses
         close socket "livecode.com|" & tSocketCount
      end repeat
   end if
end TestOpenSocketFromSameHostDifferentAddress

on TestOpenSocketFromWithAddressUsingLocalServer
   accept connections on port "8008" with message "connectedPlaceHolder"
   wait until "8008" is among the lines of the openSockets with messages

   open socket from "127.0.0.1" to "127.0.0.1:8008"
   TestAssert "open socket from with local address to local server", "127.0.0.1:8008" is among the lines of the openSockets

   repeat for each line tSocket in the openSockets
      close socket tSocket
   end repeat
end TestOpenSocketFromWithAddressUsingLocalServer

on TestOpenSocketFromWithAddressAndPortUsingLocalServer
   accept connections on port "8009" with message "connectedPlaceHolder"
   wait until "8009" is among the lines of the openSockets with messages

   open socket from "127.0.0.1:8010" to "127.0.0.1:8009"
   TestAssert "open socket from with local address and port to local server", "127.0.0.1:8009" is among the lines of the openSockets

   close socket "127.0.0.1:8010"
   close socket "127.0.0.1:8009"
   close socket "8009"
end TestOpenSocketFromWithAddressAndPortUsingLocalServer
