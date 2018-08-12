script "CoreEnginePassword"
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
   if the editionType is not "community" then
      return "SKIP Password tests for community"
   end if
end TestSetup

on TestPassword
   create stack "foo"
   
   TestAssert "stack initial password empty", the password of stack "foo" is empty
   
   TestAssert "stack initial passKey true", the passKey of stack "foo"
   
   TestAssertThrow "can't set password in community", \ 
      "SetPasswordOnStack", the long id of me, "EE_STACK_PASSWORD_NOT_SUPPORTED"
   
   TestAssertThrow "can't set passKey in community", \
      "SetPasskeyOnStack", the long id of me, "EE_STACK_PASSWORD_NOT_SUPPORTED"
   
   delete stack "foo"
end TestPassword

on SetPasswordOnStack
   set the password of stack "foo" to "bar"
end SetPasswordOnStack

on SetPasskeyOnStack
   set the passKey of stack "foo" to "bar"
end SetPasskeyOnStack
