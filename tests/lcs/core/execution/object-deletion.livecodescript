script "CoreExecutionObjectDeletion"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */


on TestSetup
   create stack "test"
end TestSetup


on TestDeleteObjectInNestedGroup
   set the defaultStack to "test"
   create group "test1"
   create group "test2" in group "test1"
   create button "test" in group "test2"
   set the script of button "test" to \
         "on DeleteIt" & return & \
         "DeleteGroup" & return & \
         "end DeleteIt"
   set the script of stack "test" to \
         "on DeleteGroup" & return & \
         "delete group "& quote & "test1" & quote & return & \
         "end DeleteGroup"
   TestAssertThrow "Delete object executing script in nested group", "DeleteIt", the long id of button "test" of stack "test", 347
end TestDeleteObjectInNestedGroup

on TestTeardown
   delete stack "test"
end TestTeardown
