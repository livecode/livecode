script "CoreStringsDelimitedChunk"
/*
Copyright (C) 2015 LiveCode Ltd.

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

on TestForwardRanges
  TestAssert "uncased delimiter front", item 1 of "foo,foobar,bar,barbaz,baz" is "foo"
  TestAssert "uncased delimiter middle", item 3 of "foo,foobar,bar,barbaz,baz" is "bar"
  TestAssert "uncased delimiter back", item 5 of "foo,foobar,bar,barbaz,baz" is "baz"
  TestAssert "uncased delimiter before", item 0 of "foo,foobar,bar,barbaz,baz" is empty
  TestAssert "uncased delimiter after", item 6 of "foo,foobar,bar,barbaz,baz" is empty

  TestAssert "uncased delimiter range front", item 1 to 2 of "foo,foobar,bar,barbaz,baz" is "foo,foobar"
  TestAssert "uncased delimiter range middle", item 2 to 4 of "foo,foobar,bar,barbaz,baz" is "foobar,bar,barbaz"
  TestAssert "uncased delimiter range back", item 4 to 5 of "foo,foobar,bar,barbaz,baz" is "barbaz,baz"
  TestAssert "uncased delimiter range before", item 0 to 2 of "foo,foobar,bar,barbaz,baz" is "foo,foobar"
  TestAssert "uncased delimiter range after", item 4 to 6 of "foo,foobar,bar,barbaz,baz" is "barbaz,baz"

  TestAssert "uncased trailing delimiter front", item 1 of "foo,foobar,bar,barbaz,baz," is "foo"
  TestAssert "uncased trailing delimiter middle", item 3 of "foo,foobar,bar,barbaz,baz," is "bar"
  TestAssert "uncased trailing delimiter back", item 5 of "foo,foobar,bar,barbaz,baz," is "baz"
  TestAssert "uncased trailing delimiter before", item 0 of "foo,foobar,bar,barbaz,baz," is empty
  TestAssert "uncased trailing delimiter after", item 6 of "foo,foobar,bar,barbaz,baz," is empty

  TestAssert "uncased trailing delimiter range front", item 1 to 2 of "foo,foobar,bar,barbaz,baz," is "foo,foobar"
  TestAssert "uncased trailing delimiter range middle", item 2 to 4 of "foo,foobar,bar,barbaz,baz," is "foobar,bar,barbaz"
  TestAssert "uncased trailing delimiter range back", item 4 to 5 of "foo,foobar,bar,barbaz,baz," is "barbaz,baz"
  TestAssert "uncased trailing delimiter range before", item 0 to 2 of "foo,foobar,bar,barbaz,baz," is "foo,foobar"
  TestAssert "uncased trailing delimiter range after", item 4 to 6 of "foo,foobar,bar,barbaz,baz," is "barbaz,baz,"
end TestForwardRanges
