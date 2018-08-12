script "com.livecode.library.getopt"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>. 
*/

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if

   insert the script of me into back

   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

/**
Title: Command-line Option Parser

Author: LiveCode

Version: 1.0.0

Description:
Command-line argument parser library.
*/

/**
Syntax: GetOpt(<grammar> [, <argumentArray>])

Summary: Parse options from command line arguments

Associated: GetOptLibrary

Example:
local tInfo
put GetOpt("h,help") into tInfo
if the number of elements in tInfo["errors"] > 0 then
   write "ERROR:" && tInfo["errors"][1] to stderr
else if "help" is among the keys of tInfo["options"] then
   write "Usage: my-program [--help]" to stderr
end if

Parameters:
grammar (string): A string describing the valid options for the program
argumentArray (array): A numerically keyed array containing arguments to be parsed

Returns (array): An array containing options, arguments and error info

Description:
Parse UNIX-like command line arguments, extracting options and their
arguments.

The <GetOpt> function parses the command-line arguments.  The optional
parameter <argumentArray> is a numerically-keyed array containing the
arguments to be parsed; if it is omitted, it is set to
`the commandArguments`.

The <grammar> describes the possible options that <GetOpt> should
understand.  It is a string containing a series of option
specifications, separated by spaces.  Each option specification is
a series of option names which should be treated as synonyms of each
other, separated by commas.  Single-character synonyms are treated
as short (`-h`) options, and multi-character synonyms are treated as
long (`--help`) options.  Option names can't start with a `-` or
contain `=`.  The last synonym in each option specification is
treated as the "proper" name of the option.

For example, `h,help` indicates that both `-h` and `--help` options
are supported, and they are both properly known as the "help" option.

Each option specification can end with a `=`.  This indicates that
the option expects an argument.  For example, with the grammar
`-o,--output=`, the option can be specified like `-o file`, `-ofile`,
`--output file`, or `--output=file`.

If a non-option argument is found in <argumentArray>, <GetOpt> stops
parsing options and all subsequent arguments are returned as they are.
`-` is always treated as a non-option argument.  The special `--`
option ends option parsing, but is otherwise ignored.  This makes it
possible to pass filenames on the command line that
are named the same as options.  For example, `-- --help`
will treat `--help` as a normal argument, not an option.

Short options can be run together.  For example, if <grammar> is
`h,help v,verbose`, then `-hv` is treated as if both `--help` and
`--verbose` were passed as options to the command.

If an option is specified more than once, only the last occurrence
is reflected in the return  value of <GetOpt>

The return value of <GetOpt> is an array with three keys:

* "options": an array where the keys are option names and the 
values are the arguments to those options.
* "arguments": a numerically-indexed array where the values are the
non-option arguments
* "errors": a numerically-indexed array where the values are error
messages from <GetOpt>'s processing of the <argumentArray>

Argument processing errors occur if:

* an option is found that wasn't in the <grammar>
* an option has an argument, but the <grammar> says it shouldn't
* an option doesn't have an argument, but the <grammar> says it should

*/
function GetOpt pGrammar, pOptionalArgs   
   -- If the user didn't provide an argument array, get the system array
   local tArgumentsA
   if paramCount() < 2 then
      put the commandArguments into tArgumentsA
   else
      put pOptionalArgs into tArgumentsA
   end if
   
   local tGrammarA
   put parseGrammar(pGrammar) into tGrammarA
   
   return scanArguments(tGrammarA, tArgumentsA)
end GetOpt

private function parseGrammar pGrammar
   local tGrammarA
   set the linedelimiter to space
   set the itemdelimiter to comma
   
   local tOptionSpec, tSubOption
   repeat for each line tOptionSpec in pGrammar
      if tOptionSpec is empty then
         next repeat
      end if
      
      -- Detect if the option requires an argument.  This is indicated
      -- by the spec ending with a "=" character
      local tHasArgument
      put false into tHasArgument
      if tOptionSpec ends with "=" then
         put true into tHasArgument
         delete char -1 of tOptionSpec
      end if
      
      -- The last name in the option specification is treated as the
      -- "real" name of the option.
      local tName
      put item -1 of tOptionSpec into tName
      if tName is empty then
         throw "GetOpt: invalid option grammar: empty option name list"
      end if
      
      put tHasArgument into tGrammarA[tName]["has_argument"]
      
      repeat for each item tSubOption in tOptionSpec
         if tSubOption is tName then
            next repeat
         end if
         
         if tSubOption is among the keys of tGrammarA then
            throw "GetOpt: invalid option grammar: reused option name"
         end if
         
         if tSubOption begins with "-" then
            throw "GetOpt: invalid option grammar: option name begins with '-'"
         end if
         
         put tName into tGrammarA[tSubOption]["alias"]
      end repeat
   end repeat
   
   return tGrammarA
end parseGrammar

private function scanArguments pGrammarA, pArgumentsA
   -- Generate an empty array
   local tResultA
   put empty into tResultA[1]
   delete variable tResultA[1]
   
   local tArg, tHasIncompleteOpt, tIncompleteOpt, tFoundEnd
   put false into tFoundEnd
   put false into tHasIncompleteOpt
   put empty into tIncompleteOpt
   
   repeat for each element tArg in pArgumentsA
      if tFoundEnd then
         arrayAppend tResultA["arguments"], tArg
         next repeat
      end if
      
      -- If the previous option still requires an argument,
      -- consume the current command line argument
      if tHasIncompleteOpt then
         put tArg into tResultA["options"][tIncompleteOpt]
         put false into tHasIncompleteOpt
         put empty into tIncompleteOpt
         next repeat
      end if
      
      -- The special "--" argument ends option processing
      if tArg is "--" then
         put true into tFoundEnd
         next repeat
      end if
      
      -- If the current argument doesn't begin with "-" then
      -- it's not an option; end option processing
      if tArg is "-" or not (tArg begins with "-") then
         arrayAppend tResultA["arguments"], tArg
         put true into tFoundEnd
         next repeat
      end if
      
      -- Process the option, repeatedly if necessary
      if tArg begins with "--" then
         processLongOption tResultA, pGrammarA, tArg, \
               tHasIncompleteOpt, tIncompleteOpt
      else
         repeat forever
            if processShortOption(tResultA, pGrammarA, tArg, \
                  tHasIncompleteOpt, tIncompleteOpt) then
               exit repeat
            end if
         end repeat
      end if
   end repeat
   
   if tHasIncompleteOpt then
      arrayAppend tResultA["errors"], merge("Missing argument for '[[tIncompleteOpt]]' option")
   end if
   
   return tResultA
end scanArguments

private command processLongOption @xResultA, pGrammarA, pOpt, \
      @rHasIncompleteOpt, @rIncompleteOpt
   
   if pOpt begins with "--" then
      delete char 1 to 2 of pOpt
   end if
   
   local tHasArgument, tName, tArgument, tArgOffset
   put offset("=", pOpt) into tArgOffset
   put char 1 to (tArgOffset - 1) of pOpt into tName
   put char (tArgOffset + 1) to -1 of pOpt into tArgument
   put ("=" is in pOpt) into tHasArgument
   
   if tName is not among the keys of pGrammarA then
      arrayAppend xResultA["errors"], merge("Unknown option '--[[tName]]'")
      return true
   end if
   
   local tRealName
   put resolveOptionAlias(pGrammarA, tName) into tRealName
   if tRealName is empty then
      arrayAppend xResultA["errors"], merge("Unknown option '-[[tName]]'")
      return true
   end if
   
   if pGrammarA[tRealName]["has_argument"] then
      if not tHasArgument then
         put tRealName into rIncompleteOpt
         put true into rHasIncompleteOpt
         return true
      end if
   else
      if tHasArgument then
         arrayAppend xResultA["errors"], merge("Unexpected argument for option '--[[tName]]'")
      else
         put empty into tArgument
      end if
   end if
   
   put tArgument into xResultA["options"][tRealName]
   
   return true
end processLongOption

private function processShortOption @xResultA, pGrammarA, @xOpt, \
      @rHasIncompleteOpt, @rIncompleteOpt
   
   if xOpt begins with "-" then
      delete char 1 of xOpt
   end if
   
   local tHasArgument, tName, tArgument
   put char 1 of xOpt into tName
   put char 2 to -1 of xOpt into tArgument
   put (tArgument is not empty) into tHasArgument
   
   local tRealName
   put resolveOptionAlias(pGrammarA, tName) into tRealName
   if tRealName is empty then
      arrayAppend xResultA["errors"], merge("Unknown option '-[[tName]]'")
      return true
   end if
   
   local tMoreOptions
   put false into tMoreOptions
   if pGrammarA[tRealName]["has_argument"] then
      if not tHasArgument then
         put tRealName into rIncompleteOpt
         put true into rHasIncompleteOpt
         return true
      end if
   else
      if tHasArgument then
         put tArgument into xOpt
         put empty into tArgument
         put true into tMoreOptions
      end if
      put empty into tArgument
   end if
   
   put tArgument into xResultA["options"][tRealName]
   
   return not tMoreOptions
end processShortOption

private function resolveOptionAlias pGrammarA, pName
   if pName is not among the keys of pGrammarA then
      return empty
   end if
   
   repeat while "alias" is among the keys of pGrammarA[pName]
      put pGrammarA[pName]["alias"] into pName
   end repeat
   return pName
end resolveOptionAlias

private command arrayAppend @xArray, pValue
   put pValue into xArray[1 + the number of elements in xArray]
end arrayAppend
