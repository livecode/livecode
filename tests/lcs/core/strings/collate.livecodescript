script "CoreStringsCollate"
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

on TestCollateCaseSensitve
  set the caseSensitive to true
  TestAssert "collate case sensitive lowercase", _collate_("xy","XY") is -1
  TestAssert "collate case sensitive uppercase", _collate_("XY","XY") is 0
  TestAssert "collate case sensitive mixedcase", _collate_("xY","XY") is -1

  set the caseSensitive to false
  TestAssert "collate case insensitive lowercase", _collate_("xy","XY") is 0
  TestAssert "collate case insensitive uppercase", _collate_("XY","XY") is 0
  TestAssert "collate case insensitive mixedcase", _collate_("xY","XY") is 0
end TestCollateCaseSensitve

on TestCollateNumeric
  local tOptions
  put true into tOptions["numeric"]
  TestAssert "collate numeric", _collate_("xy9a","xy10b", tOptions) is -1
end TestCollateNumeric

on TestCollateIgnorePunctuation
  local tOptions
  put true into tOptions["ignorepunctuation"]
  TestAssert "collate ignore punctuation", _collate_("xy..a","xy,b", tOptions) is -1
end TestCollateIgnorePunctuation

