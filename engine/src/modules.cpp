/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "prefix.h"

// This file exists only to hold a reference to every other built-in module
// to simplify the linking process for Win32

extern "C"
{
    
    struct builtin_module_descriptor {};
    extern builtin_module_descriptor __com_livecode_foreign_module_info;
    extern builtin_module_descriptor __com_livecode_arithmetic_module_info;
    extern builtin_module_descriptor __com_livecode_array_module_info;
    extern builtin_module_descriptor __com_livecode_binary_module_info;
    extern builtin_module_descriptor __com_livecode_bitwise_module_info;
    extern builtin_module_descriptor __com_livecode_byte_module_info;
    extern builtin_module_descriptor __com_livecode_char_module_info;
    extern builtin_module_descriptor __com_livecode_date_module_info;
    extern builtin_module_descriptor __com_livecode_encoding_module_info;
    extern builtin_module_descriptor __com_livecode_file_module_info;
    extern builtin_module_descriptor __com_livecode_item_module_info;
    extern builtin_module_descriptor __com_livecode_line_module_info;
    extern builtin_module_descriptor __com_livecode_list_module_info;
    extern builtin_module_descriptor __com_livecode_logic_module_info;
    extern builtin_module_descriptor __com_livecode_math_module_info;
    extern builtin_module_descriptor __com_livecode_segmentchunk_module_info;
    extern builtin_module_descriptor __com_livecode_sort_module_info;
    extern builtin_module_descriptor __com_livecode_stream_module_info;
    extern builtin_module_descriptor __com_livecode_string_module_info;
    extern builtin_module_descriptor __com_livecode_system_module_info;
    extern builtin_module_descriptor __com_livecode_type_module_info;
    extern builtin_module_descriptor __com_livecode_typeconvert_module_info;
    extern builtin_module_descriptor __com_livecode_mathfoundation_module_info;
    extern builtin_module_descriptor __com_livecode_encoding_module_info;
    extern builtin_module_descriptor __com_livecode_canvas_module_info;
    extern builtin_module_descriptor __com_livecode_engine_module_info;
    extern builtin_module_descriptor __com_livecode_widget_module_info;
    
    builtin_module_descriptor* g_builtin_modules[] =
    {
        &__com_livecode_foreign_module_info,
        &__com_livecode_arithmetic_module_info,
        &__com_livecode_array_module_info,
        &__com_livecode_binary_module_info,
        &__com_livecode_bitwise_module_info,
        &__com_livecode_byte_module_info,
        &__com_livecode_char_module_info,
        &__com_livecode_date_module_info,
        //&__com_livecode_encoding_module_info,
        &__com_livecode_file_module_info,
        //&__com_livecode_item_module_info,
        //&__com_livecode_line_module_info,
        &__com_livecode_list_module_info,
        &__com_livecode_logic_module_info,
        &__com_livecode_math_module_info,
        //&__com_livecode_segmentchunk_module_info,
        &__com_livecode_sort_module_info,
        &__com_livecode_stream_module_info,
        &__com_livecode_string_module_info,
        &__com_livecode_type_module_info,
        &__com_livecode_typeconvert_module_info,
        &__com_livecode_mathfoundation_module_info,
        &__com_livecode_encoding_module_info,
        &__com_livecode_canvas_module_info,
        &__com_livecode_engine_module_info,
        &__com_livecode_widget_module_info,
    };
    
    unsigned int g_builtin_module_count = sizeof(g_builtin_modules) / sizeof(builtin_module_descriptor*);
    
    extern void (*MCArithmeticExecAddIntegerToInteger)();
    extern void (*MCArrayEvalKeysOf)();
    extern void (*MCBinaryEvalConcatenateBytes)();
    extern void (*MCBitwiseEvalBitwiseAnd)();
    extern void (*MCByteEvalNumberOfBytesIn)();
    extern void (*MCCharEvalNumberOfCharsIn)();
    extern void (*MCDateExecGetLocalTime)();
    extern void (*MCFileExecGetContents)();
    extern void (*MCListEvalHeadOf)();
    extern void (*MCLogicEvalNot)();
    extern void (*MCMathEvalRealToPowerOfReal)();
    extern void (*MCSortExecSortListAscendingText)();
    extern void (*MCStreamExecWriteToStream)();
    extern void (*MCStringEvalConcatenate)();
    extern void (*MCTypeEvalIsDefined)();
    extern void (*MCTypeConvertExecSplitStringByDelimiter)();
    extern void (*MCMathFoundationEvalFloorNumber)();
    extern void (*MCEncodingExecDecodeUsingBinary)();
    extern void (*MCWidgetExecRedrawAll)();
    extern void (*MCEngineExecResolveScriptObject)();
    extern void (*MCCanvasThisCanvas)();
    
    // Pull in a reference to all of the module-*.cpp objects too
    void *g_builtin_ptrs[] =
    {
        &MCArithmeticExecAddIntegerToInteger,
        &MCArrayEvalKeysOf,
        &MCBinaryEvalConcatenateBytes,
        &MCBitwiseEvalBitwiseAnd,
        &MCByteEvalNumberOfBytesIn,
        &MCCharEvalNumberOfCharsIn,
        &MCDateExecGetLocalTime,
        &MCFileExecGetContents,
        &MCListEvalHeadOf,
        &MCLogicEvalNot,
        &MCMathEvalRealToPowerOfReal,
        &MCSortExecSortListAscendingText,
        &MCStreamExecWriteToStream,
        &MCStringEvalConcatenate,
        &MCTypeEvalIsDefined,
        &MCTypeConvertExecSplitStringByDelimiter,
        &MCMathFoundationEvalFloorNumber,
        &MCEncodingExecDecodeUsingBinary,
        &MCWidgetExecRedrawAll,
        &MCCanvasThisCanvas,
        &MCEngineExecResolveScriptObject,
    };
    
}

extern bool MCForeignModuleInitialize(void);
extern bool MCCanvasModuleInitialize(void);
extern bool MCEngineModuleInitialize(void);
bool MCModulesInitialize(void)
{
    if (!MCForeignModuleInitialize())
        return false;
    if (!MCCanvasModuleInitialize())
        return false;
    if (!MCEngineModuleInitialize())
        return false;
    return true;
}

extern void MCForeignModuleFinalize(void);
extern void MCCanvasModuleFinalize(void);
extern void MCEngineModuleFinalize(void);
void MCModulesFinalize(void)
{
    MCEngineModuleFinalize();
    MCCanvasModuleFinalize();
    MCForeignModuleFinalize();
}
