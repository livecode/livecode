script "TestMimeEncode"
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

on TestSetup
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/mime/mime.livecodescript" \
         into tLibFilename

   start using stack tLibFilename
end TestSetup

on TestEncodeQuotedPrintable
   __TestEncode "If you believe that truth=beauty, then surely mathematics is the most beautiful branch of philosophy.", \
         "If you believe that truth=3Dbeauty, then surely mathematics is the most bea=" & crlf & "utiful branch of philosophy.", \
         "Equals sign"
   
   __TestEncode "Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Nam liber tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum. Typi non habent claritatem insitam; est usus legentis in iis qui facit eorum claritatem. Investigationes demonstraverunt lectores legere me lius quod ii legunt saepius. Claritas est etiam processus dynamicus, qui sequitur mutationem consuetudium lectorum. Mirum est notare quam littera gothica, quam nunc putamus parum claram, anteposuerit litterarum formas humanitatis per seacula quarta decima et quinta decima. Eodem modo typi, qui nunc nobis videntur parum clari, fiant sollemnes in futurum.", \
         "Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy =" & crlf & \
         "nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wi=" & crlf & \
         "si enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lo=" & crlf & \
         "bortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure d=" & crlf & \
         "olor in hendrerit in vulputate velit esse molestie consequat, vel illum dol=" & crlf & \
         "ore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio digni=" & crlf & \
         "ssim qui blandit praesent luptatum zzril delenit augue duis dolore te feuga=" & crlf & \
         "it nulla facilisi. Nam liber tempor cum soluta nobis eleifend option congue=" & crlf & \
         " nihil imperdiet doming id quod mazim placerat facer possim assum. Typi non=" & crlf & \
         " habent claritatem insitam; est usus legentis in iis qui facit eorum clarit=" & crlf & \
         "atem. Investigationes demonstraverunt lectores legere me lius quod ii legun=" & crlf & \
         "t saepius. Claritas est etiam processus dynamicus, qui sequitur mutationem =" & crlf & \
         "consuetudium lectorum. Mirum est notare quam littera gothica, quam nunc put=" & crlf & \
         "amus parum claram, anteposuerit litterarum formas humanitatis per seacula q=" & crlf & \
         "uarta decima et quinta decima. Eodem modo typi, qui nunc nobis videntur par=" & crlf & \
         "um clari, fiant sollemnes in futurum.", \
         "Line length 76 columns"
   
   __TestEncode "foo ", "foo=20", "Trailing space"
   
   __TestEncode "foo" & tab, "foo=09", "Trailing tab"
   
   __TestEncode "foo" & crlf & "bar", "foo=0D=0Abar", "Meaningful CRLF"
   
   __TestEncode "foo" & return & "bar", "foo" & crlf & "bar", "Line breaks"
   
   __TestEncode "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ", \
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=20", \
         "Exactly 74 chars of which the last one is space"
   
   __TestEncode "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ", \
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=" & crlf &"=20", \
         "Exactly 75 chars of which the last one is space"
   
   __TestEncode "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ", \
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=" & crlf &"=20", \
         "Exactly 76 chars of which the last one is space"
   
   __TestEncode "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx ", \
         "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx=" & crlf &"x=20", \
         "Exactly 77 chars of which the last one is space"
   
   __TestEncode "¯\_(ツ)_/¯", "=C2=AF\_(=E3=83=84)_/=C2=AF", "Encode non ascii chars"
end TestEncodeQuotedPrintable

private command __TestEncode pOriginal, pExpected, pMessage
   local tEncoded
   put mimeEncodeForMIMETransfer(pOriginal, "quoted-printable") into tEncoded
   TestAssert pMessage, tEncoded is textEncode(pExpected,"UTF-8")
end __TestEncode
