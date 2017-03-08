/*                                                                     -*-c++-*-
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

/* Compatibility functions for instantiating Mac platform implementations. */

extern MCPlatform::Ref<MCPlatformColorTransform>
    MCMacPlatformCreateColorTransform();

extern MCPlatform::Ref<MCPlatformCursor> MCMacPlatformCreateCursor(void);

extern MCPlatform::Ref<MCPlatformLoadedFont>
MCMacPlatformCreateLoadedFont();

extern MCPlatform::Ref<MCPlatformSound> MCMacPlatformCreateSound(void);

extern MCPlatform::Ref<MCPlatformWindowMask> MCMacPlatformCreateWindowMask(void);

extern void MCMacPlatformHideCursorUntilMouseMoves(void);

extern MCPlatform::Ref<MCPlatformPrintDialogSession> MCMacPlatformCreatePrintDialogSession(void);

extern MCPlatform::Ref<MCPlatformCore> MCMacPlatformCreateCore(void);
