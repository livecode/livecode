script "ObjectRepositoryLibrary"
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
	TestLoadExtension "com.livecode.library.objectrepository"
end TestSetup

on TestObjectRepository
   local tObject
   create stack uuid()
   put it into tObject
   
   local tID
   put objectRepositoryStore(tObject) into tID
   TestAssert "object stored in repository", tID is an integer
   TestAssert "object stored in repository once", tID is objectRepositoryStore(tObject)
   
   local tRetrieved
   put objectRepositoryRetrieve(tID) into tRetrieved
   TestAssert "object retrieved from repository", tRetrieved is tObject
   
   local tObject2
   put uuid() into tObject2
   set the name of tObject to tObject2
   put objectRepositoryRetrieve(tID) into tRetrieved
   TestAssert "mutated object retrieved from repository", \
         tRetrieved is not empty and \
         exists(tRetrieved) and \
         not exists(tObject)
   put tRetrieved into tObject
   
   objectRepositoryRemove tID
   TestAssert "object removed from repository", objectRepositoryRetrieve(tID) is empty
   
   put objectRepositoryStore(tObject) into tID
   delete tObject
   TestAssert "deleted object removed from repository", objectRepositoryRetrieve(tID) is empty
   
   -- because of the behavior of objectRepositoryRetrieve it's not possible to test objectRepositoryFlush
end TestObjectRepository
