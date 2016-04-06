/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_SYNTAX__
#define __MC_SYNTAX__

#ifndef __MC_EXEC__
#include "exec.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCSyntaxHandlerType
{
	kMCSyntaxHandlerTypeNone,
	kMCSyntaxHandlerTypeMessage,
	kMCSyntaxHandlerTypeBeforeMessage,
	kMCSyntaxHandlerTypeAfterMessage,
	kMCSyntaxHandlerTypeCommand,
	kMCSyntaxHandlerTypePrivateCommand,
	kMCSyntaxHandlerTypeFunction,
	kMCSyntaxHandlerTypePrivateFunction,
	kMCSyntaxHandlerTypeSetProp,
	kMCSyntaxHandlerTypeGetProp,
};

////////////////////////////////////////////////////////////////////////////////

typedef struct MCSyntaxFactory *MCSyntaxFactoryRef;

bool MCSyntaxFactoryCreate(MCSyntaxFactoryRef& r_factory);
void MCSyntaxFactoryDestroy(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryCopyLog(MCSyntaxFactoryRef factory, MCStringRef& r_log);

bool MCSyntaxFactoryIsValid(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryEvalConstant(MCSyntaxFactoryRef factory, MCValueRef value);
void MCSyntaxFactoryEvalConstantOldString(MCSyntaxFactoryRef factory, const MCString& string);
void MCSyntaxFactoryEvalConstantUInt(MCSyntaxFactoryRef factory, uinteger_t value);
void MCSyntaxFactoryEvalConstantInt(MCSyntaxFactoryRef factory, integer_t value);
void MCSyntaxFactoryEvalConstantDouble(MCSyntaxFactoryRef factory, double value);
void MCSyntaxFactoryEvalConstantLegacyPoint(MCSyntaxFactoryRef factory, MCPoint value);
void MCSyntaxFactoryEvalConstantLegacyRectangle(MCSyntaxFactoryRef factory, MCRectangle p_value);
void MCSyntaxFactoryEvalConstantEnum(MCSyntaxFactoryRef factory, const MCExecEnumTypeInfo *enum_info, intenum_t value);
void MCSyntaxFactoryEvalConstantNil(MCSyntaxFactoryRef factory);
void MCSyntaxFactoryEvalConstantBool(MCSyntaxFactoryRef factory, bool value);

void MCSyntaxFactoryDefineGlobal(MCSyntaxFactoryRef factory, MCNameRef name);
void MCSyntaxFactoryDefineLocal(MCSyntaxFactoryRef factory, MCNameRef name, MCValueRef value);
void MCSyntaxFactoryDefineConstant(MCSyntaxFactoryRef factory, MCNameRef name, MCValueRef value);

void MCSyntaxFactoryBeginHandlerList(MCSyntaxFactoryRef factory);
void MCSyntaxFactoryEndHandlerList(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryBeginHandler(MCSyntaxFactoryRef factory, MCSyntaxHandlerType type, MCNameRef name);
void MCSyntaxFactoryEndHandler(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryBeginStatement(MCSyntaxFactoryRef factory, integer_t line, integer_t pos);
void MCSyntaxFactoryEndStatement(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryBeginExpression(MCSyntaxFactoryRef factory, integer_t line, integer_t pos);
void MCSyntaxFactoryEndExpression(MCSyntaxFactoryRef factory);

void MCSyntaxFactoryDefineParameter(MCSyntaxFactoryRef factory, MCNameRef name, bool is_ref);

void MCSyntaxFactoryExecUnimplemented(MCSyntaxFactoryRef factory);
void MCSyntaxFactoryExecMethod(MCSyntaxFactoryRef factory, const MCExecMethodInfo *method);
void MCSyntaxFactoryExecMethodWithArgs(MCSyntaxFactoryRef factory, const MCExecMethodInfo *method, ...);

void MCSyntaxFactoryEvalUnimplemented(MCSyntaxFactoryRef factory);
void MCSyntaxFactoryEvalMethod(MCSyntaxFactoryRef factory, const MCExecMethodInfo *method);
void MCSyntaxFactoryEvalMethodWithArgs(MCSyntaxFactoryRef factory, const MCExecMethodInfo *method, ...);
void MCSyntaxFactoryEvalResult(MCSyntaxFactoryRef factory);
void MCSyntaxFactoryEvalList(MCSyntaxFactoryRef factory, uindex_t count);

////////////////////////////////////////////////////////////////////////////////

#endif
