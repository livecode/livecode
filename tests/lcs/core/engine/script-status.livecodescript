script "CoreEngineScriptStatus"
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

on TestScriptStatus
   create stack "foo"
   
   set the script of stack "foo" to \
         "on bar" & return & \
         "   ,fooBar" & return & \
         "end bar"
   
   TestAssert "script status is error when script is bad", \
         the scriptStatus of stack "foo" is "error" and the result is not empty
   
   set the script of stack "foo" to \
         "on bar" & return & \
         "   fooBar" & return & \
         "end bar"
   
   TestAssert "script status is compiled when script is good", \
         the scriptStatus of stack "foo" is "compiled" and the result is empty
   
   delete stack "foo"
   
   TestSkipIfNot "ide"
   
   get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revaskdialog.rev")
      
   TestAssert "script status is uncompiled when stack is only loaded", \
            the scriptStatus of card 1 of it is "uncompiled"
end TestScriptStatus
