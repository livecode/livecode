script "CoreDebugging_Log"
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

on startup
    set the logMessage to $2
    log $3
    quit 0
end startup

command log pValue
    write "Error: this should not be called" & return to stderr
    quit 1
end log

command logPublic pValue
    write pValue & return to stdout
end logPublic

private command logPrivate pValue
    write pValue & return to stdout
end logPrivate
