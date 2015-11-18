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

on TestForwardOrdinals
  TestAssert "uncased delimiter first", the first item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "aa"
  TestAssert "uncased delimiter second", the second item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "bb"
  TestAssert "uncased delimiter third", the third item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "cc"
  TestAssert "uncased delimiter fourth", the fourth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "dd"
  TestAssert "uncased delimiter fifth", the fifth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "ee"
  TestAssert "uncased delimiter sixth", the sixth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "ff"
  TestAssert "uncased delimiter seventh", the seventh item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "gg"
  TestAssert "uncased delimiter eighth", the eighth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "hh"
  TestAssert "uncased delimiter ninth", the ninth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "ii"
  TestAssert "uncased delimiter tenth", the tenth item of "aa,bb,cc,dd,ee,ff,gg,hh,ii,jj" is "jj"
end TestForwardOrdinals

on TestForwardSingletons
  TestAssert "uncased delimiter front", item 1 of "foo,foobar,bar,barbaz,baz" is "foo"
  TestAssert "uncased delimiter middle", item 3 of "foo,foobar,bar,barbaz,baz" is "bar"
  TestAssert "uncased delimiter back", item 5 of "foo,foobar,bar,barbaz,baz" is "baz"
  TestAssert "uncased delimiter before", item 0 of "foo,foobar,bar,barbaz,baz" is empty
  TestAssert "uncased delimiter after", item 6 of "foo,foobar,bar,barbaz,baz" is empty

  TestAssert "uncased trailing delimiter front", item 1 of "foo,foobar,bar,barbaz,baz," is "foo"
  TestAssert "uncased trailing delimiter middle", item 3 of "foo,foobar,bar,barbaz,baz," is "bar"
  TestAssert "uncased trailing delimiter back", item 5 of "foo,foobar,bar,barbaz,baz," is "baz"
  TestAssert "uncased trailing delimiter before", item 0 of "foo,foobar,bar,barbaz,baz," is empty
  TestAssert "uncased trailing delimiter after", item 6 of "foo,foobar,bar,barbaz,baz," is empty

  set the itemDelimiter to "<>"

  TestAssert "uncased multidelimiter front", item 1 of "foo<>foobar<>bar<>barbaz<>baz" is "foo"
  TestAssert "uncased multidelimiter middle", item 3 of "foo<>foobar<>bar<>barbaz<>baz" is "bar"
  TestAssert "uncased multidelimiter back", item 5 of "foo<>foobar<>bar<>barbaz<>baz" is "baz"
  TestAssert "uncased multidelimiter before", item 0 of "foo<>foobar<>bar<>barbaz<>baz" is empty
  TestAssert "uncased multidelimiter after", item 6 of "foo<>foobar<>bar<>barbaz<>baz" is empty

  TestAssert "uncased trailing multidelimiter front", item 1 of "foo<>foobar<>bar<>barbaz<>baz<>" is "foo"
  TestAssert "uncased trailing multidelimiter middle", item 3 of "foo<>foobar<>bar<>barbaz<>baz<>" is "bar"
  TestAssert "uncased trailing multidelimiter back", item 5 of "foo<>foobar<>bar<>barbaz<>baz<>" is "baz"
  TestAssert "uncased trailing multidelimiter before", item 0 of "foo<>foobar<>bar<>barbaz<>baz<>" is empty
  TestAssert "uncased trailing multidelimiter after", item 6 of "foo<>foobar<>bar<>barbaz<>baz<>" is empty

  set the itemDelimiter to "!!"

  TestAssert "uncased symmetric multidelimiter front", item 1 of "foo!!foobar!!bar!!barbaz!!baz" is "foo"
  TestAssert "uncased symmetric multidelimiter middle", item 3 of "foo!!foobar!!bar!!barbaz!!baz" is "bar"
  TestAssert "uncased symmetric multidelimiter back", item 5 of "foo!!foobar!!bar!!barbaz!!baz" is "baz"
  TestAssert "uncased symmetric multidelimiter before", item 0 of "foo!!foobar!!bar!!barbaz!!baz" is empty
  TestAssert "uncased symmetric multidelimiter after", item 6 of "foo!!foobar!!bar!!barbaz!!baz" is empty

  TestAssert "uncased trailing symmetric multidelimiter front", item 1 of "foo!!foobar!!bar!!barbaz!!baz!!" is "foo"
  TestAssert "uncased trailing symmetric multidelimiter middle", item 3 of "foo!!foobar!!bar!!barbaz!!baz!!" is "bar"
  TestAssert "uncased trailing symmetric multidelimiter back", item 5 of "foo!!foobar!!bar!!barbaz!!baz!!" is "baz"
  TestAssert "uncased trailing symmetric multidelimiter before", item 0 of "foo!!foobar!!bar!!barbaz!!baz!!" is empty
  TestAssert "uncased trailing symmetric multidelimiter after", item 6 of "foo!!foobar!!bar!!barbaz!!baz!!" is empty

  set the itemDelimiter to "!!"

  TestAssert "uncased overlapping symmetric multidelimiter front", item 1 of "foo!!!foobar!!!bar!!!barbaz!!!baz" is "foo"
  TestAssert "uncased overlapping symmetric multidelimiter middle", item 3 of "foo!!!foobar!!!bar!!!barbaz!!!baz" is "!bar"
  TestAssert "uncased overlapping symmetric multidelimiter back", item 5 of "foo!!!foobar!!!bar!!!barbaz!!!baz" is "!baz"
  TestAssert "uncased overlapping symmetric multidelimiter before", item 0 of "foo!!!foobar!!!bar!!!barbaz!!!baz" is empty
  TestAssert "uncased overlapping symmetric multidelimiter after", item 6 of "foo!!!foobar!!!bar!!!barbaz!!!baz" is empty

  TestAssert "uncased trailing overlapping symmetric multidelimiter front", item 1 of "foo!!!foobar!!!bar!!!barbaz!!!baz!!" is "foo"
  TestAssert "uncased trailing overlapping overlapping symmetric multidelimiter middle", item 3 of "foo!!!foobar!!!bar!!!barbaz!!!baz!!" is "!bar"
  TestAssert "uncased trailing overlapping symmetric multidelimiter back", item 5 of "foo!!!foobar!!!bar!!!barbaz!!!baz!!" is "!baz"
  TestAssert "uncased trailing overlapping symmetric multidelimiter before", item 0 of "foo!!!foobar!!!bar!!!barbaz!!!baz!!" is empty
  TestAssert "uncased trailing overlapping symmetric multidelimiter after", item 6 of "foo!!!foobar!!!bar!!!barbaz!!!baz!!" is empty
end TestForwardSingletons

on TestForwardRanges
  TestAssert "uncased delimiter range front", item 1 to 2 of "foo,foobar,bar,barbaz,baz" is "foo,foobar"
  TestAssert "uncased delimiter range middle", item 2 to 4 of "foo,foobar,bar,barbaz,baz" is "foobar,bar,barbaz"
  TestAssert "uncased delimiter range back", item 4 to 5 of "foo,foobar,bar,barbaz,baz" is "barbaz,baz"
  TestAssert "uncased delimiter range before", item 0 to 2 of "foo,foobar,bar,barbaz,baz" is "foo,foobar"
  TestAssert "uncased delimiter range after", item 4 to 6 of "foo,foobar,bar,barbaz,baz" is "barbaz,baz"

  TestAssert "uncased trailing delimiter range front", item 1 to 2 of "foo,foobar,bar,barbaz,baz," is "foo,foobar"
  TestAssert "uncased trailing delimiter range middle", item 2 to 4 of "foo,foobar,bar,barbaz,baz," is "foobar,bar,barbaz"
  TestAssert "uncased trailing delimiter range back", item 4 to 5 of "foo,foobar,bar,barbaz,baz," is "barbaz,baz"
  TestAssert "uncased trailing delimiter range before", item 0 to 2 of "foo,foobar,bar,barbaz,baz," is "foo,foobar"
  TestAssert "uncased trailing delimiter range after", item 4 to 6 of "foo,foobar,bar,barbaz,baz," is "barbaz,baz,"
end TestForwardRanges

on TestForwardSubSingletons
  set the lineDelimiter to space
  TestAssert "uncased delimiter sub front", item 1 of line 2 of "a foo,foobar,bar,barbaz,baz b" is "foo"
  TestAssert "uncased delimiter sub middle", item 3 of line 2 of "a foo,foobar,bar,barbaz,baz b" is "bar"
  TestAssert "uncased delimiter sub back", item 5 of line 2 of "a foo,foobar,bar,barbaz,baz b" is "baz"
  TestAssert "uncased delimiter sub before", item 0 of line 2 of "a foo,foobar,bar,barbaz,baz b" is empty
  TestAssert "uncased delimiter sub after", item 6 of line 2 of "a foo,foobar,bar,barbaz,baz b" is empty
end TestForwardSubSingletons
