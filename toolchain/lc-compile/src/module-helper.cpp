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



// This file exists only to hold a reference to every other built-in module
// to simplify the linking process for Win32


extern "C"
{
    
struct builtin_module_descriptor {};
extern builtin_module_descriptor __com_livecode_foreign_module_info;
extern builtin_module_descriptor __com_livecode_arithmetic_module_info;
extern builtin_module_descriptor __com_livecode_array_module_info;
extern builtin_module_descriptor __com_livecode_assert_module_info;
extern builtin_module_descriptor __com_livecode_binary_module_info;
extern builtin_module_descriptor __com_livecode_bitwise_module_info;
extern builtin_module_descriptor __com_livecode_byte_module_info;
extern builtin_module_descriptor __com_livecode_char_module_info;
extern builtin_module_descriptor __com_livecode_codeunit_module_info;
extern builtin_module_descriptor __com_livecode_date_module_info;
extern builtin_module_descriptor __com_livecode_encoding_module_info;
extern builtin_module_descriptor __com_livecode_file_module_info;
extern builtin_module_descriptor __com_livecode_item_module_info;
extern builtin_module_descriptor __com_livecode_line_module_info;
extern builtin_module_descriptor __com_livecode_list_module_info;
extern builtin_module_descriptor __com_livecode_logic_module_info;
extern builtin_module_descriptor __com_livecode_math_module_info;
extern builtin_module_descriptor __com_livecode_mathfoundation_module_info;
extern builtin_module_descriptor __com_livecode_segmentchunk_module_info;
extern builtin_module_descriptor __com_livecode_sort_module_info;
extern builtin_module_descriptor __com_livecode_stream_module_info;
extern builtin_module_descriptor __com_livecode_string_module_info;
extern builtin_module_descriptor __com_livecode_system_module_info;
extern builtin_module_descriptor __com_livecode_type_module_info;
extern builtin_module_descriptor __com_livecode_typeconvert_module_info;
extern builtin_module_descriptor __com_livecode_unittest_module_info;
extern builtin_module_descriptor __com_livecode_unittest___IMPL_module_info;

builtin_module_descriptor* g_builtin_modules[] =
{
    &__com_livecode_foreign_module_info,
    &__com_livecode_arithmetic_module_info,
    &__com_livecode_array_module_info,
    &__com_livecode_assert_module_info,
    &__com_livecode_binary_module_info,
    &__com_livecode_bitwise_module_info,
    &__com_livecode_byte_module_info,
    &__com_livecode_char_module_info,
    &__com_livecode_codeunit_module_info,
    &__com_livecode_date_module_info,
    //&__com_livecode_encoding_module_info,
    &__com_livecode_file_module_info,
    //&__com_livecode_item_module_info,
    //&__com_livecode_line_module_info,
    &__com_livecode_list_module_info,
    &__com_livecode_logic_module_info,
    &__com_livecode_math_module_info,
    &__com_livecode_mathfoundation_module_info,
    //&__com_livecode_segmentchunk_module_info,
    &__com_livecode_sort_module_info,
    &__com_livecode_stream_module_info,
    &__com_livecode_string_module_info,
    &__com_livecode_system_module_info,
    &__com_livecode_type_module_info,
    &__com_livecode_typeconvert_module_info,
    &__com_livecode_unittest_module_info,
    &__com_livecode_unittest___IMPL_module_info,
};

unsigned int g_builtin_module_count = sizeof(g_builtin_modules) / sizeof(builtin_module_descriptor*);

extern void (*MCArithmeticExecAddIntegerToInteger)();
extern void (*MCArrayEvalKeysOf)();
extern void (*MCBinaryEvalConcatenateBytes)();
extern void (*MCBitwiseEvalBitwiseAnd)();
extern void (*MCByteEvalNumberOfBytesIn)();
extern void (*MCCharEvalNumberOfCharsIn)();
extern void (*MCCodeunitEvalNumberOfCodeunitsIn)();
extern void (*MCDateExecGetLocalDate)();
extern void (*MCFileExecGetContents)();
extern void (*MCListEvalHeadOf)();
extern void (*MCLogicEvalNot)();
extern void (*MCMathEvalRealToPowerOfReal)();
extern void (*MCMathFoundationExecRoundRealToNearest)();
extern void (*MCSortExecSortListAscendingText)();
extern void (*MCStreamExecWriteToStream)();
extern void (*MCStringEvalConcatenate)();
extern void (*MCSystemExecGetOperatingSystem)();
extern void (*MCTypeEvalIsEmpty)();
extern void (*MCTypeConvertExecSplitStringByDelimiter)();

// Pull in a reference to all of the module-*.cpp objects too
void *g_builtin_ptrs[] =
{
    &MCArithmeticExecAddIntegerToInteger,
    &MCArrayEvalKeysOf,
    &MCBinaryEvalConcatenateBytes,
    &MCBitwiseEvalBitwiseAnd,
    &MCByteEvalNumberOfBytesIn,
    &MCCharEvalNumberOfCharsIn,
    &MCCodeunitEvalNumberOfCodeunitsIn,
    &MCDateExecGetLocalDate,
    &MCFileExecGetContents,
    &MCListEvalHeadOf,
    &MCLogicEvalNot,
    &MCMathEvalRealToPowerOfReal,
    &MCMathFoundationExecRoundRealToNearest,
    &MCSortExecSortListAscendingText,
    &MCStreamExecWriteToStream,
    &MCStringEvalConcatenate,
    &MCSystemExecGetOperatingSystem,
    &MCTypeEvalIsEmpty,
    &MCTypeConvertExecSplitStringByDelimiter
};

}

extern bool MCForeignModuleInitialize(void);
extern bool MCMathModuleInitialize(void);

bool MCModulesInitialize(void)
{
    if (!MCForeignModuleInitialize())
        return false;
	if (!MCMathModuleInitialize())
		return false;
    return true;
}

extern void MCForeignModuleFinalize(void);
extern void MCMathModuleFinalize(void);

void MCModulesFinalize(void)
{
	MCMathModuleFinalize();
    MCForeignModuleFinalize();
}
