script "CoreEnginePut"
/*
Copyright (C) 2018 LiveCode Ltd.

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

// Test MCVariable::modify variant
on TestPutAfterData
	local tVar
	put numToByte(1) into tVar
	put numToByte(1) after tVar
	TestAssert "put after preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterData

// Test MCEngineExecPutIntoVariable ValueRef chunk variant
on TestPutAfterDataChunk
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) after byte 1 of tVar
	TestAssert "put after chunk preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataChunk

// Test MCVariable::modify_ctxt variant
on TestPutAfterDataExecValue
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) into tArray[1]
	put tArray[1] after tVar
	TestAssert "put as execvalue after preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataExecValue

// Test MCEngineExecPutIntoVariable ExecValue chunk variant
on TestPutAfterDataChunkExecValue
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) into tArray[1]
	put tArray[1] after byte 1 of tVar
	TestAssert "put as execvalue after chunk preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataChunkExecValue