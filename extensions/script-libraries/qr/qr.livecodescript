script "com.livecode.library.qr"
/*

This library is in the public domain.
There are no restrictions whatsoever.

Original Author - John Craig

*/

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back
   __Initialize
   
   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: QR Code Generator

Version: 2.0.0

Author: LiveCode

Description:
The QR Code Generator library for LiveCode generates QR codes
according to the ISO international standard (ISO/IEC18004).

'QR Code' is a registered trademark of DENSO WAVE INCORPORATED.

All versions (1 - 40) and all error correction levels
(L, M, Q, H) are supported.

SVGIcon: M384 1024L384 1152 256 1152 256 1024 384 1024ZM384 256L384 384 256 384 256 256 384 256ZM1152 256L1152 384 1024 384 1024 256 1152 256ZM128 1279L512 1279 512 896 128 896 128 1279ZM128 512L512 512 512 128 128 128 128 512ZM896 512L1280 512 1280 128 896 128 896 512ZM640 768L640 1408 0 1408 0 768 640 768ZM1152 1280L1152 1408 1024 1408 1024 1280 1152 1280ZM1408 1280L1408 1408 1280 1408 1280 1280 1408 1280ZM1408 768L1408 1152 1024 1152 1024 1024 896 1024 896 1408 768 1408 768 768 1152 768 1152 896 1280 896 1280 768 1408 768ZM640 0L640 640 0 640 0 0 640 0ZM1408 0L1408 640 768 640 768 0 1408 0Z

Tags:
barcode

*/

local sRemCount, sVersion, sQRSize, sImgSize, sQZSize, sQRCodeId
local sChar0, sChar255, sBlackPixel, sWhitePixel, sImgData, sAlphaData
local sAlignPosList, sCapacityList, sCCIList, sFormatList, sVersionList, sVersionList2
local sModeNumeric, sModeAlpha, sModeByte, sModeKanji, sMode, sModeNamesA, sAlphaChars
local sBitStream, sByteStream, sECC, sECCDataList
local sMsgPolyA, sECCPolyA, sGF256List, sGenPolyAlphaExpList
local sSymbolMap, sBlankMap, sDataMap
local sColorNames, sPNGBlack, sPNGWhite


# _crc32
local sCrcTableA, sCrcTableCreated


private command __Initialize
   put 0 into sRemCount
   
   # default quiet zone size
   put 4 into sQZSize
   
   # character codes used for generating image data
   put numToChar(0) into sChar0
   put numToChar(255) into sChar255
   put sChar0 & sChar0 & sChar0 & sChar0 into sBlackPixel
   put sChar0 & sChar255 & sChar255 & sChar255 into sWhitePixel
   put sChar0 & sChar0 & sChar0 into sPNGBlack
   put sChar255 & sChar255 & sChar255 into sPNGWhite
   
   local tTmpString
   # alignment positioning
   put "1,0" & LF after tTmpString
   put "2,1,6,18" & LF after tTmpString
   put "3,1,6,22" & LF after tTmpString
   put "4,1,6,26" & LF after tTmpString
   put "5,1,6,30" & LF after tTmpString
   put "6,1,6,34" & LF after tTmpString
   put "7,6,6,22,38" & LF after tTmpString
   put "8,6,6,24,42" & LF after tTmpString
   put "9,6,6,26,46" & LF after tTmpString
   put "10,6,6,28,50" & LF after tTmpString
   put "11,6,6,30,54" & LF after tTmpString
   put "12,6,6,32,58" & LF after tTmpString
   put "13,6,6,34,62" & LF after tTmpString
   put "14,13,6,26,46,66" & LF after tTmpString
   put "15,13,6,26,48,70" & LF after tTmpString
   put "16,13,6,26,50,74" & LF after tTmpString
   put "17,13,6,30,54,78" & LF after tTmpString
   put "18,13,6,30,56,82" & LF after tTmpString
   put "19,13,6,30,58,86" & LF after tTmpString
   put "20,13,6,34,62,90" & LF after tTmpString
   put "21,22,6,28,50,72,94" & LF after tTmpString
   put "22,22,6,26,50,74,98" & LF after tTmpString
   put "23,22,6,30,54,78,102" & LF after tTmpString
   put "24,22,6,28,54,80,106" & LF after tTmpString
   put "25,22,6,32,58,84,110" & LF after tTmpString
   put "26,22,6,30,58,86,114" & LF after tTmpString
   put "27,22,6,34,62,90,118" & LF after tTmpString
   put "28,33,6,26,50,74,98,122" & LF after tTmpString
   put "29,33,6,30,54,78,102,126" & LF after tTmpString
   put "30,33,6,26,52,78,104,130" & LF after tTmpString
   put "31,33,6,30,56,82,108,134" & LF after tTmpString
   put "32,33,6,34,60,86,112,138" & LF after tTmpString
   put "33,33,6,30,58,86,114,142" & LF after tTmpString
   put "34,33,6,34,62,90,118,146" & LF after tTmpString
   put "35,46,6,30,54,78,102,126,150" & LF after tTmpString
   put "36,46,6,24,50,76,102,128,154" & LF after tTmpString
   put "37,46,6,28,54,80,106,132,158" & LF after tTmpString
   put "38,46,6,32,58,84,110,136,162" & LF after tTmpString
   put "39,46,6,26,54,82,110,138,166" & LF after tTmpString
   put "40,46,6,30,58,86,114,142,170" after tTmpString
   put tTmpString into sAlignPosList
   put empty into tTmpString
   
   # version / ECC capacity
   # version, codewords, remainder, *L, *M, *Q, *H (* = error correction codewords)
   put "1,26,0,7,10,13,17" & LF after tTmpString
   put "2,44,7,10,16,22,28" & LF after tTmpString
   put "3,70,7,15,26,36,44" & LF after tTmpString
   put "4,100,7,20,36,52,64" & LF after tTmpString
   put "5,134,7,26,48,72,88" & LF after tTmpString
   put "6,172,7,36,64,96,112" & LF after tTmpString
   put "7,196,0,40,72,108,130" & LF after tTmpString
   put "8,242,0,48,88,132,156" & LF after tTmpString
   put "9,292,0,60,110,160,192" & LF after tTmpString
   put "10,346,0,72,130,192,224" & LF after tTmpString
   put "11,404,0,80,150,224,264" & LF after tTmpString
   put "12,466,0,96,176,260,308" & LF after tTmpString
   put "13,532,0,104,198,288,352" & LF after tTmpString
   put "14,581,3,120,216,320,384" & LF after tTmpString
   put "15,655,3,132,240,360,432" & LF after tTmpString
   put "16,733,3,144,280,408,480" & LF after tTmpString
   put "17,815,3,168,308,448,532" & LF after tTmpString
   put "18,901,3,180,338,504,588" & LF after tTmpString
   put "19,991,3,196,364,546,650" & LF after tTmpString
   put "20,1085,3,224,416,600,700" & LF after tTmpString
   put "21,1156,4,224,442,644,750" & LF after tTmpString
   put "22,1258,4,252,476,690,816" & LF after tTmpString
   put "23,1364,4,270,504,750,900" & LF after tTmpString
   put "24,1474,4,300,560,810,960" & LF after tTmpString
   put "25,1588,4,312,588,870,1050" & LF after tTmpString
   put "26,1706,4,336,644,952,1110" & LF after tTmpString
   put "27,1828,4,360,700,1020,1200" & LF after tTmpString
   put "28,1921,3,390,728,1050,1260" & LF after tTmpString
   put "29,2051,3,420,784,1140,1350" & LF after tTmpString
   put "30,2185,3,450,812,1200,1440" & LF after tTmpString
   put "31,2323,3,480,868,1290,1530" & LF after tTmpString
   put "32,2465,3,510,924,1350,1620" & LF after tTmpString
   put "33,2611,3,540,980,1440,1710" & LF after tTmpString
   put "34,2761,3,570,1036,1530,1800" & LF after tTmpString
   put "35,2876,0,570,1064,1590,1890" & LF after tTmpString
   put "36,3034,0,600,1120,1680,1980" & LF after tTmpString
   put "37,3196,0,630,1204,1770,2100" & LF after tTmpString
   put "38,3362,0,660,1260,1860,2220" & LF after tTmpString
   put "39,3532,0,720,1316,1950,2310" & LF after tTmpString
   put "40,3706,0,750,1372,2040,2430" after tTmpString
   put tTmpString into sCapacityList
   put empty into tTmpString
   
   # ECC data
   # version-ECC, total ECC words, no. of blocks, total words per block, data words per block, no. of blocks, total words per block, data words per block
   put "1-L,7,1,26,19" & LF after tTmpString
   put "1-M,10,1,26,16" & LF after tTmpString
   put "1-Q,13,1,26,13" & LF after tTmpString
   put "1-H,17,1,26,9" & LF after tTmpString
   put "2-L,10,1,44,34" & LF after tTmpString
   put "2-M,16,1,44,28" & LF after tTmpString
   put "2-Q,22,1,44,22" & LF after tTmpString
   put "2-H,28,1,44,16" & LF after tTmpString
   put "3-L,15,1,70,55" & LF after tTmpString
   put "3-M,26,1,70,44" & LF after tTmpString
   put "3-Q,36,2,35,17" & LF after tTmpString
   put "3-H,44,2,35,13" & LF after tTmpString
   put "4-L,20,1,100,80" & LF after tTmpString
   put "4-M,36,2,50,32" & LF after tTmpString
   put "4-Q,52,2,50,24" & LF after tTmpString
   put "4-H,64,4,25,9" & LF after tTmpString
   put "5-L,26,1,134,108" & LF after tTmpString
   put "5-M,48,2,67,43" & LF after tTmpString
   put "5-Q,72,2,33,15,2,34,16" & LF after tTmpString
   put "5-H,88,2,33,11,2,34,12" & LF after tTmpString
   put "6-L,36,2,86,68" & LF after tTmpString
   put "6-M,64,4,43,27" & LF after tTmpString
   put "6-Q,96,4,43,19" & LF after tTmpString
   put "6-H,112,4,43,15" & LF after tTmpString
   put "7-L,40,2,98,78" & LF after tTmpString
   put "7-M,72,4,49,31" & LF after tTmpString
   put "7-Q,108,2,32,14,4,33,15" & LF after tTmpString
   put "7-H,130,4,39,13,1,40,14" & LF after tTmpString
   put "8-L,48,2,121,97" & LF after tTmpString
   put "8-M,88,2,60,38,2,61,39" & LF after tTmpString
   put "8-Q,132,4,40,18,2,41,19" & LF after tTmpString
   put "8-H,156,4,40,14,2,41,15" & LF after tTmpString
   put "9-L,60,2,146,116" & LF after tTmpString
   put "9-M,110,3,58,36,2,59,37" & LF after tTmpString
   put "9-Q,160,4,36,16,4,37,17" & LF after tTmpString
   put "9-H,192,4,36,12,4,37,13" & LF after tTmpString
   put "10-L,72,2,86,68,2,87,69" & LF after tTmpString
   put "10-M,130,4,69,43,1,70,44" & LF after tTmpString
   put "10-Q,192,6,43,19,2,44,20" & LF after tTmpString
   put "10-H,224,6,43,15,2,44,16" & LF after tTmpString
   put "11-L,80,4,101,81" & LF after tTmpString
   put "11-M,150,1,80,50,4,81,51" & LF after tTmpString
   put "11-Q,224,4,50,22,4,51,23" & LF after tTmpString
   put "11-H,264,3,36,12,8,37,13" & LF after tTmpString
   put "12-L,96,2,116,92,2,117,93" & LF after tTmpString
   put "12-M,176,6,58,36,2,59,37" & LF after tTmpString
   put "12-Q,260,4,46,20,6,47,21" & LF after tTmpString
   put "12-H,308,7,42,14,4,43,15" & LF after tTmpString
   put "13-L,104,4,133,107" & LF after tTmpString
   put "13-M,198,8,59,37,1,60,38" & LF after tTmpString
   put "13-Q,288,8,44,20,4,45,21" & LF after tTmpString
   put "13-H,352,12,33,11,4,34,12" & LF after tTmpString
   put "14-L,120,3,145,115,1,146,116" & LF after tTmpString
   put "14-M,216,4,64,40,5,65,41" & LF after tTmpString
   put "14-Q,320,11,36,16,5,37,17" & LF after tTmpString
   put "14-H,384,11,36,12,5,37,13" & LF after tTmpString
   put "15-L,132,5,109,87,1,110,88" & LF after tTmpString
   put "15-M,240,5,65,41,5,66,42" & LF after tTmpString
   put "15-Q,360,5,54,24,7,55,25" & LF after tTmpString
   put "15-H,432,11,36,12,7,37,13" & LF after tTmpString
   put "16-L,144,5,122,98,1,123,99" & LF after tTmpString
   put "16-M,280,7,73,45,3,74,46" & LF after tTmpString
   put "16-Q,408,15,43,19,2,44,20" & LF after tTmpString
   put "16-H,480,3,45,15,13,46,16" & LF after tTmpString
   put "17-L,168,1,135,107,5,136,108" & LF after tTmpString
   put "17-M,308,10,74,46,1,75,47" & LF after tTmpString
   put "17-Q,448,1,50,22,15,51,23" & LF after tTmpString
   put "17-H,532,2,42,14,17,43,15" & LF after tTmpString
   put "18-L,180,5,150,120,1,151,121" & LF after tTmpString
   put "18-M,338,9,69,43,4,70,44" & LF after tTmpString
   put "18-Q,504,17,50,22,1,51,23" & LF after tTmpString
   put "18-H,588,2,42,14,19,43,15" & LF after tTmpString
   put "19-L,196,3,141,113,4,142,114" & LF after tTmpString
   put "19-M,364,3,70,44,11,71,45" & LF after tTmpString
   put "19-Q,546,17,47,21,4,48,22" & LF after tTmpString
   put "19-H,650,9,39,13,16,40,14" & LF after tTmpString
   put "20-L,224,3,135,107,5,136,108" & LF after tTmpString
   put "20-M,416,3,67,41,13,68,42" & LF after tTmpString
   put "20-Q,600,15,54,24,5,55,25" & LF after tTmpString
   put "20-H,700,15,43,15,10,44,16" & LF after tTmpString
   put "21-L,22,4,144,116,4,145,117" & LF after tTmpString
   put "21-M,442,17,68,42" & LF after tTmpString
   put "21-Q,644,17,50,22,6,51,23" & LF after tTmpString
   put "21-H,750,19,46,16,6,47,17" & LF after tTmpString
   put "22-L,252,2,139,111,7,140,112" & LF after tTmpString
   put "22-M,476,17,74,46" & LF after tTmpString
   put "22-Q,690,7,54,24,16,55,25" & LF after tTmpString
   put "22-H,816,34,37,13" & LF after tTmpString
   put "23-L,270,4,151,121,5,152,122" & LF after tTmpString
   put "23-M,504,4,75,47,14,76,48" & LF after tTmpString
   put "23-Q,750,11,54,24,14,55,25" & LF after tTmpString
   put "23-H,900,16,45,15,14,46,16" & LF after tTmpString
   put "24-L,300,6,147,117,4,148,118" & LF after tTmpString
   put "24-M,560,6,73,45,14,74,46" & LF after tTmpString
   put "24-Q,810,11,54,24,16,55,25" & LF after tTmpString
   put "24-H,960,30,46,16,2,47,17" & LF after tTmpString
   put "25-L,312,8,132,106,4,133,107" & LF after tTmpString
   put "25-M,588,8,75,47,13,76,48" & LF after tTmpString
   put "25-Q,870,7,54,24,22,55,25" & LF after tTmpString
   put "25-H,1050,22,45,15,13,46,16" & LF after tTmpString
   put "26-L,336,10,142,114,2,143,115" & LF after tTmpString
   put "26-M,644,19,74,46,4,75,47" & LF after tTmpString
   put "26-Q,952,28,50,22,6,51,23" & LF after tTmpString
   put "26-H,1110,33,46,16,4,47,17" & LF after tTmpString
   put "27-L,360,8,152,122,4,153,123" & LF after tTmpString
   put "27-M,700,22,73,45,3,74,46" & LF after tTmpString
   put "27-Q,1020,8,53,23,26,54,24" & LF after tTmpString
   put "27-H,1200,12,45,15,28,46,16" & LF after tTmpString
   put "28-L,390,3,147,117,10,148,118" & LF after tTmpString
   put "28-M,728,3,73,45,23,74,46" & LF after tTmpString
   put "28-Q,1050,4,54,24,31,55,25" & LF after tTmpString
   put "28-H,1260,11,45,15,31,46,16" & LF after tTmpString
   put "29-L,420,7,146,116,7,147,117" & LF after tTmpString
   put "29-M,784,21,73,45,7,74,46" & LF after tTmpString
   put "29-Q,1140,1,53,23,37,54,24" & LF after tTmpString
   put "29-H,1350,19,45,15,26,46,16" & LF after tTmpString
   put "30-L,450,5,145,115,10,146,116" & LF after tTmpString
   put "30-M,812,19,75,47,10,76,48" & LF after tTmpString
   put "30-Q,1200,15,54,24,25,55,25" & LF after tTmpString
   put "30-H,1440,23,45,15,25,46,16" & LF after tTmpString
   put "31-L,480,13,145,115,3,146,116" & LF after tTmpString
   put "31-M,868,2,74,46,29,75,47" & LF after tTmpString
   put "31-Q,1290,42,54,24,1,55,25" & LF after tTmpString
   put "31-H,1530,23,45,15,28,46,16" & LF after tTmpString
   put "32-L,510,17,145,115" & LF after tTmpString
   put "32-M,924,10,74,46,23,75,47" & LF after tTmpString
   put "32-Q,1350,10,54,24,35,55,25" & LF after tTmpString
   put "32-H,1620,19,45,15,35,46,16" & LF after tTmpString
   put "33-L,540,17,145,115,1,146,116" & LF after tTmpString
   put "33-M,980,14,74,46,21,75,47" & LF after tTmpString
   put "33-Q,1440,29,54,24,19,55,25" & LF after tTmpString
   put "33-H,1710,11,45,15,46,46,16" & LF after tTmpString
   put "34-L,570,13,145,115,6,146,116" & LF after tTmpString
   put "34-M,1036,14,74,46,23,75,47" & LF after tTmpString
   put "34-Q,1530,44,54,24,7,55,25" & LF after tTmpString
   put "34-H,1800,59,46,16,1,47,17" & LF after tTmpString
   put "35-L,570,12,151,121,7,152,122" & LF after tTmpString
   put "35-M,1064,12,75,47,26,76,48" & LF after tTmpString
   put "35-Q,1590,39,54,24,14,55,25" & LF after tTmpString
   put "35-H,1890,22,45,15,41,46,16" & LF after tTmpString
   put "36-L,600,6,151,121,14,152,122" & LF after tTmpString
   put "36-M,1120,6,75,47,34,76,48" & LF after tTmpString
   put "36-Q,1680,46,54,24,10,55,25" & LF after tTmpString
   put "36-H,1980,2,45,15,64,46,16" & LF after tTmpString
   put "37-L,630,17,152,122,4,153,123" & LF after tTmpString
   put "37-M,1204,29,74,46,14,75,47" & LF after tTmpString
   put "37-Q,1770,49,54,24,10,55,25" & LF after tTmpString
   put "37-H,2100,24,45,15,46,46,16" & LF after tTmpString
   put "38-L,660,4,152,122,18,153,123" & LF after tTmpString
   put "38-M,1260,13,74,46,32,75,47" & LF after tTmpString
   put "38-Q,1860,48,54,24,14,55,25" & LF after tTmpString
   put "38-H,2220,42,45,15,32,46,16" & LF after tTmpString
   put "39-L,720,20,147,117,4,148,118" & LF after tTmpString
   put "39-M,1316,40,75,47,7,76,48" & LF after tTmpString
   put "39-Q,1950,43,54,24,22,55,25" & LF after tTmpString
   put "39-H,2310,10,45,15,67,46,16" & LF after tTmpString
   put "40-L,750,19,148,118,6,149,119" & LF after tTmpString
   put "40-M,1372,18,75,47,31,76,48" & LF after tTmpString
   put "40-Q,2040,34,54,24,34,55,25" & LF after tTmpString
   put "40-H,2430,20,45,15,61,46,16" after tTmpString
   put tTmpString into sECCDataList
   put empty into tTmpString
   
   # mode indicators
   put 1 into sModeNumeric
   put 2 into sModeAlpha
   put 4 into sModeByte
   put 8 into sModeKanji
   
   # mode names
   put "Numeric" into sModeNamesA[1]
   put "Alphanumeric" into sModeNamesA[2]
   put "8 bit byte" into sModeNamesA[4]
   
   # character count indicator
   # min version, max version, numeric mode, alpha mode, 8-bit mode, Kanji mode
   put "1,9,10,9,8,8" & LF after tTmpString
   put "10,26,12,11,16,10" & LF after tTmpString
   put "27,40,14,13,16,12" after tTmpString
   put tTmpString into sCCIList
   put empty into tTmpString
   
   # alphanumeric mode characters (codes 0 - 44)
   put "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:" into sAlphaChars
   
   # log antilog table for Galois Field 256
   # exp/integer lookup, integer, exponent
   put "1,2,0" & LF after tTmpString
   put "2,4,1" & LF after tTmpString
   put "3,8,25" & LF after tTmpString
   put "4,16,2" & LF after tTmpString
   put "5,32,50" & LF after tTmpString
   put "6,64,26" & LF after tTmpString
   put "7,128,198" & LF after tTmpString
   put "8,29,3" & LF after tTmpString
   put "9,58,223" & LF after tTmpString
   put "10,116,51" & LF after tTmpString
   put "11,232,238" & LF after tTmpString
   put "12,205,27" & LF after tTmpString
   put "13,135,104" & LF after tTmpString
   put "14,19,199" & LF after tTmpString
   put "15,38,75" & LF after tTmpString
   put "16,76,4" & LF after tTmpString
   put "17,152,100" & LF after tTmpString
   put "18,45,224" & LF after tTmpString
   put "19,90,14" & LF after tTmpString
   put "20,180,52" & LF after tTmpString
   put "21,117,141" & LF after tTmpString
   put "22,234,239" & LF after tTmpString
   put "23,201,129" & LF after tTmpString
   put "24,143,28" & LF after tTmpString
   put "25,3,193" & LF after tTmpString
   put "26,6,105" & LF after tTmpString
   put "27,12,248" & LF after tTmpString
   put "28,24,200" & LF after tTmpString
   put "29,48,8" & LF after tTmpString
   put "30,96,76" & LF after tTmpString
   put "31,192,113" & LF after tTmpString
   put "32,157,5" & LF after tTmpString
   put "33,39,138" & LF after tTmpString
   put "34,78,101" & LF after tTmpString
   put "35,156,47" & LF after tTmpString
   put "36,37,225" & LF after tTmpString
   put "37,74,36" & LF after tTmpString
   put "38,148,15" & LF after tTmpString
   put "39,53,33" & LF after tTmpString
   put "40,106,53" & LF after tTmpString
   put "41,212,147" & LF after tTmpString
   put "42,181,142" & LF after tTmpString
   put "43,119,218" & LF after tTmpString
   put "44,238,240" & LF after tTmpString
   put "45,193,18" & LF after tTmpString
   put "46,159,130" & LF after tTmpString
   put "47,35,69" & LF after tTmpString
   put "48,70,29" & LF after tTmpString
   put "49,140,181" & LF after tTmpString
   put "50,5,194" & LF after tTmpString
   put "51,10,125" & LF after tTmpString
   put "52,20,106" & LF after tTmpString
   put "53,40,39" & LF after tTmpString
   put "54,80,249" & LF after tTmpString
   put "55,160,185" & LF after tTmpString
   put "56,93,201" & LF after tTmpString
   put "57,186,154" & LF after tTmpString
   put "58,105,9" & LF after tTmpString
   put "59,210,120" & LF after tTmpString
   put "60,185,77" & LF after tTmpString
   put "61,111,228" & LF after tTmpString
   put "62,222,114" & LF after tTmpString
   put "63,161,166" & LF after tTmpString
   put "64,95,6" & LF after tTmpString
   put "65,190,191" & LF after tTmpString
   put "66,97,139" & LF after tTmpString
   put "67,194,98" & LF after tTmpString
   put "68,153,102" & LF after tTmpString
   put "69,47,221" & LF after tTmpString
   put "70,94,48" & LF after tTmpString
   put "71,188,253" & LF after tTmpString
   put "72,101,226" & LF after tTmpString
   put "73,202,152" & LF after tTmpString
   put "74,137,37" & LF after tTmpString
   put "75,15,179" & LF after tTmpString
   put "76,30,16" & LF after tTmpString
   put "77,60,145" & LF after tTmpString
   put "78,120,34" & LF after tTmpString
   put "79,240,136" & LF after tTmpString
   put "80,253,54" & LF after tTmpString
   put "81,231,208" & LF after tTmpString
   put "82,211,148" & LF after tTmpString
   put "83,187,206" & LF after tTmpString
   put "84,107,143" & LF after tTmpString
   put "85,214,150" & LF after tTmpString
   put "86,177,219" & LF after tTmpString
   put "87,127,189" & LF after tTmpString
   put "88,254,241" & LF after tTmpString
   put "89,225,210" & LF after tTmpString
   put "90,223,19" & LF after tTmpString
   put "91,163,92" & LF after tTmpString
   put "92,91,131" & LF after tTmpString
   put "93,182,56" & LF after tTmpString
   put "94,113,70" & LF after tTmpString
   put "95,226,64" & LF after tTmpString
   put "96,217,30" & LF after tTmpString
   put "97,175,66" & LF after tTmpString
   put "98,67,182" & LF after tTmpString
   put "99,134,163" & LF after tTmpString
   put "100,17,195" & LF after tTmpString
   put "101,34,72" & LF after tTmpString
   put "102,68,126" & LF after tTmpString
   put "103,136,110" & LF after tTmpString
   put "104,13,107" & LF after tTmpString
   put "105,26,58" & LF after tTmpString
   put "106,52,40" & LF after tTmpString
   put "107,104,84" & LF after tTmpString
   put "108,208,250" & LF after tTmpString
   put "109,189,133" & LF after tTmpString
   put "110,103,186" & LF after tTmpString
   put "111,206,61" & LF after tTmpString
   put "112,129,202" & LF after tTmpString
   put "113,31,94" & LF after tTmpString
   put "114,62,155" & LF after tTmpString
   put "115,124,159" & LF after tTmpString
   put "116,248,10" & LF after tTmpString
   put "117,237,21" & LF after tTmpString
   put "118,199,121" & LF after tTmpString
   put "119,147,43" & LF after tTmpString
   put "120,59,78" & LF after tTmpString
   put "121,118,212" & LF after tTmpString
   put "122,236,229" & LF after tTmpString
   put "123,197,172" & LF after tTmpString
   put "124,151,115" & LF after tTmpString
   put "125,51,243" & LF after tTmpString
   put "126,102,167" & LF after tTmpString
   put "127,204,87" & LF after tTmpString
   put "128,133,7" & LF after tTmpString
   put "129,23,112" & LF after tTmpString
   put "130,46,192" & LF after tTmpString
   put "131,92,247" & LF after tTmpString
   put "132,184,140" & LF after tTmpString
   put "133,109,128" & LF after tTmpString
   put "134,218,99" & LF after tTmpString
   put "135,169,13" & LF after tTmpString
   put "136,79,103" & LF after tTmpString
   put "137,158,74" & LF after tTmpString
   put "138,33,222" & LF after tTmpString
   put "139,66,237" & LF after tTmpString
   put "140,132,49" & LF after tTmpString
   put "141,21,197" & LF after tTmpString
   put "142,42,254" & LF after tTmpString
   put "143,84,24" & LF after tTmpString
   put "144,168,227" & LF after tTmpString
   put "145,77,165" & LF after tTmpString
   put "146,154,153" & LF after tTmpString
   put "147,41,119" & LF after tTmpString
   put "148,82,38" & LF after tTmpString
   put "149,164,184" & LF after tTmpString
   put "150,85,180" & LF after tTmpString
   put "151,170,124" & LF after tTmpString
   put "152,73,17" & LF after tTmpString
   put "153,146,68" & LF after tTmpString
   put "154,57,146" & LF after tTmpString
   put "155,114,217" & LF after tTmpString
   put "156,228,35" & LF after tTmpString
   put "157,213,32" & LF after tTmpString
   put "158,183,137" & LF after tTmpString
   put "159,115,46" & LF after tTmpString
   put "160,230,55" & LF after tTmpString
   put "161,209,63" & LF after tTmpString
   put "162,191,209" & LF after tTmpString
   put "163,99,91" & LF after tTmpString
   put "164,198,149" & LF after tTmpString
   put "165,145,188" & LF after tTmpString
   put "166,63,207" & LF after tTmpString
   put "167,126,205" & LF after tTmpString
   put "168,252,144" & LF after tTmpString
   put "169,229,135" & LF after tTmpString
   put "170,215,151" & LF after tTmpString
   put "171,179,178" & LF after tTmpString
   put "172,123,220" & LF after tTmpString
   put "173,246,252" & LF after tTmpString
   put "174,241,190" & LF after tTmpString
   put "175,255,97" & LF after tTmpString
   put "176,227,242" & LF after tTmpString
   put "177,219,86" & LF after tTmpString
   put "178,171,211" & LF after tTmpString
   put "179,75,171" & LF after tTmpString
   put "180,150,20" & LF after tTmpString
   put "181,49,42" & LF after tTmpString
   put "182,98,93" & LF after tTmpString
   put "183,196,158" & LF after tTmpString
   put "184,149,132" & LF after tTmpString
   put "185,55,60" & LF after tTmpString
   put "186,110,57" & LF after tTmpString
   put "187,220,83" & LF after tTmpString
   put "188,165,71" & LF after tTmpString
   put "189,87,109" & LF after tTmpString
   put "190,174,65" & LF after tTmpString
   put "191,65,162" & LF after tTmpString
   put "192,130,31" & LF after tTmpString
   put "193,25,45" & LF after tTmpString
   put "194,50,67" & LF after tTmpString
   put "195,100,216" & LF after tTmpString
   put "196,200,183" & LF after tTmpString
   put "197,141,123" & LF after tTmpString
   put "198,7,164" & LF after tTmpString
   put "199,14,118" & LF after tTmpString
   put "200,28,196" & LF after tTmpString
   put "201,56,23" & LF after tTmpString
   put "202,112,73" & LF after tTmpString
   put "203,224,236" & LF after tTmpString
   put "204,221,127" & LF after tTmpString
   put "205,167,12" & LF after tTmpString
   put "206,83,111" & LF after tTmpString
   put "207,166,246" & LF after tTmpString
   put "208,81,108" & LF after tTmpString
   put "209,162,161" & LF after tTmpString
   put "210,89,59" & LF after tTmpString
   put "211,178,82" & LF after tTmpString
   put "212,121,41" & LF after tTmpString
   put "213,242,157" & LF after tTmpString
   put "214,249,85" & LF after tTmpString
   put "215,239,170" & LF after tTmpString
   put "216,195,251" & LF after tTmpString
   put "217,155,96" & LF after tTmpString
   put "218,43,134" & LF after tTmpString
   put "219,86,177" & LF after tTmpString
   put "220,172,187" & LF after tTmpString
   put "221,69,204" & LF after tTmpString
   put "222,138,62" & LF after tTmpString
   put "223,9,90" & LF after tTmpString
   put "224,18,203" & LF after tTmpString
   put "225,36,89" & LF after tTmpString
   put "226,72,95" & LF after tTmpString
   put "227,144,176" & LF after tTmpString
   put "228,61,156" & LF after tTmpString
   put "229,122,169" & LF after tTmpString
   put "230,244,160" & LF after tTmpString
   put "231,245,81" & LF after tTmpString
   put "232,247,11" & LF after tTmpString
   put "233,243,245" & LF after tTmpString
   put "234,251,22" & LF after tTmpString
   put "235,235,235" & LF after tTmpString
   put "236,203,122" & LF after tTmpString
   put "237,139,117" & LF after tTmpString
   put "238,11,44" & LF after tTmpString
   put "239,22,215" & LF after tTmpString
   put "240,44,79" & LF after tTmpString
   put "241,88,174" & LF after tTmpString
   put "242,176,213" & LF after tTmpString
   put "243,125,233" & LF after tTmpString
   put "244,250,230" & LF after tTmpString
   put "245,233,231" & LF after tTmpString
   put "246,207,173" & LF after tTmpString
   put "247,131,232" & LF after tTmpString
   put "248,27,116" & LF after tTmpString
   put "249,54,214" & LF after tTmpString
   put "250,108,244" & LF after tTmpString
   put "251,216,234" & LF after tTmpString
   put "252,173,168" & LF after tTmpString
   put "253,71,80" & LF after tTmpString
   put "254,142,88" & LF after tTmpString
   put "255,1,175" after tTmpString
   put tTmpString into sGF256List
   put empty into tTmpString
   
   # generator polynomial alpha exponents for each number of error correction codewords
   put "7,0,87,229,146,149,238,102,21" & LF after tTmpString
   put "10,0,251,67,46,61,118,70,64,94,32,45" & LF after tTmpString
   put "13,0,74,152,176,100,86,100,106,104,130,218,206,140,78" & LF after tTmpString
   put "15,0,8,183,61,91,202,37,51,58,58,237,140,124,5,99,105" & LF after tTmpString
   put "16,0,120,104,107,109,102,161,76,3,91,191,147,169,182,194,225,120" & LF after tTmpString
   put "17,0,43,139,206,78,43,239,123,206,214,147,24,99,150,39,243,163,136" & LF after tTmpString
   put "18,0,215,234,158,94,184,97,118,170,79,187,152,148,252,179,5,98,96,153" & LF after tTmpString
   put "20,0,17,60,79,50,61,163,26,187,202,180,221,225,83,239,156,164,212,212,188,190" & LF after tTmpString
   put "22,0,210,171,247,242,93,230,14,109,221,53,200,74,8,172,98,80,219,134,160,105,165,231" & LF after tTmpString
   put "24,0,229,121,135,48,211,117,251,126,159,180,169,152,192,226,228,218,111,0,117,232,87,96,227,21" & LF after tTmpString
   put "26,0,173,125,158,2,103,182,118,17,145,201,111,28,165,53,161,21,245,142,13,102,48,227,153,145,218,70" & LF after tTmpString
   put "28,0,168,223,200,104,224,234,108,180,110,190,195,147,205,27,232,201,21,43,245,87,42,195,212,119,242,37,9,123" & LF after tTmpString
   put "30,0,41,173,145,152,216,31,179,182,50,48,110,86,239,96,222,125,42,173,226,193,224,130,156,37,251,216,238,40,192,180" & LF after tTmpString
   put "32,0,10,6,106,190,249,167,4,67,209,138,138,32,242,123,89,27,120,185,80,156,38,69,171,60,28,222,80,52,254,185,220,241" & LF after tTmpString
   put "34,0,111,77,146,94,26,21,108,19,105,94,113,193,86,140,163,125,58,158,229,239,218,103,56,70,114,61,183,129,167,13,98,62,129,51" & LF after tTmpString
   put "36,0,200,183,98,16,172,31,246,234,60,152,115,0,167,152,113,248,238,107,18,63,218,37,87,210,105,177,120,74,121,196,117,251,113,233,30,120" & LF after tTmpString
   put "40,0,59,116,79,161,252,98,128,205,128,161,247,57,163,56,235,106,53,26,187,174,226,104,170,7,175,35,181,114,88,41,47,163,125,134,72,20,232,53,35,15" & LF after tTmpString
   put "42,0,250,103,221,230,25,18,137,231,0,3,58,242,221,191,110,84,230,8,188,106,96,147,15,131,139,34,101,223,39,101,213,199,237,254,201,123,171,162,194,117,50,96" & LF after tTmpString
   put "44,0,190,7,61,121,71,246,69,55,168,188,89,243,191,25,72,123,9,145,14,247,,238,44,78,143,62,224,126,118,114,68,163,52,194,217,147,204,169,37,130,113,102,73,181" & LF after tTmpString
   put "46,0,112,94,88,112,253,224,202,115,187,99,89,5,54,113,129,44,58,16,135,216,169,211,36,,4,96,60,241,73,104,234,8,249,245,119,174,52,25,157,224,43,202,223,19,82,15" & LF after tTmpString
   put "48,0,228,25,196,130,211,146,60,24,251,90,39,102,240,61,178,63,46,123,115,18,221,111,135,160,182,205,107,206,95,150,120,184,91,21,247,156,140,238,191,11,94,227,84,50,163,39,34,108" & LF after tTmpString
   put "50,0,232,125,157,161,164,9,118,46,209,99,203,193,35,3,209,111,195,242,203,225,46,13,32,160,126,209,130,160,242,215,242,75,77,42,189,32,113,65,124,69,228,114,235,175,124,170,215,232,133,205" & LF after tTmpString
   put "52,0,116,50,86,186,50,220,251,89,192,46,86,127,124,19,184,233,151,215,22,14,59,145,37,242,203,134,254,89,190,94,59,65,124,113,100,233,235,121,22,76,86,97,39,242,200,220,101,33,239,254,116,51" & LF after tTmpString
   put "54,0,183,26,201,87,210,221,113,21,46,65,45,50,238,184,249,225,102,58,209,218,109,165,26,95,184,192,52,245,35,254,238,175,172,79,123,25,122,43,120,108,215,80,128,201,235,8,153,59,101,31,198,76,31,156" & LF after tTmpString
   put "56,0,106,120,107,157,164,216,112,116,2,91,248,163,36,201,202,229,6,144,254,155,135,208,170,209,12,139,127,142,182,249,177,174,190,28,10,85,239,184,101,124,152,206,96,23,163,61,27,196,247,151,154,202,207,20,61,10" & LF after tTmpString
   put "58,0,82,116,26,247,66,27,62,107,252,182,200,185,235,55,251,242,210,144,154,237,176,141,192,248,152,249,206,85,253,142,65,165,125,23,24,30,122,240,214,6,129,218,29,145,127,134,206,245,117,29,41,63,159,142,233,125,148,123" & LF after tTmpString
   put "60,0,107,140,26,12,9,141,243,197,226,197,219,45,211,101,219,120,28,181,127,6,100,247,2,205,198,57,115,219,101,109,160,82,37,38,238,49,160,209,121,86,11,124,30,181,84,25,194,87,65,102,190,220,70,27,209,16,89,7,33,240" & LF after tTmpString
   put "62,0,65,202,113,98,71,223,248,118,214,94,0,122,37,23,2,228,58,121,7,105,135,78,243,118,70,76,223,89,72,50,70,111,194,17,212,126,181,35,221,117,235,11,229,149,147,123,213,40,115,6,200,100,26,246,182,218,127,215,36,186,110,106" & LF after tTmpString
   put "64,0,45,51,175,9,7,158,159,49,68,119,92,123,177,204,187,254,200,78,141,149,119,26,127,53,160,93,199,212,29,24,145,156,208,150,218,209,4,216,91,47,184,146,47,140,195,195,125,242,238,63,99,108,140,230,242,31,204,11,178,243,217,156,213,231" & LF after tTmpString
   put "66,0,5,118,222,180,136,136,162,51,46,117,13,215,81,17,139,247,197,171,95,173,65,137,178,68,111,95,101,41,72,214,169,197,95,7,44,154,77,111,236,40,121,143,63,87,80,253,240,126,217,77,34,232,106,50,168,82,76,146,67,106,171,25,132,93,45,105" & LF after tTmpString
   put "68,0,247,159,223,33,224,93,77,70,90,160,32,254,43,150,84,101,190,205,133,52,60,202,165,220,203,151,93,84,15,84,253,173,160,89,227,52,199,97,95,231,52,177,41,125,137,241,166,225,118,2,54,32,82,215,175,198,43,238,235,27,101,184,127,3,5,8,163,238" after tTmpString
   put tTmpString into sGenPolyAlphaExpList
   put empty into tTmpString
   
   # symbol format data
   # ECC & mask code, data bits (LSB -> MSB)
   put "L0,111011111000100" & LF after tTmpString
   put "L1,111001011110011" & LF after tTmpString
   put "L2,111110110101010" & LF after tTmpString
   put "L3,111100010011101" & LF after tTmpString
   put "L4,110011000101111" & LF after tTmpString
   put "L5,110001100011000" & LF after tTmpString
   put "L6,110110001000001" & LF after tTmpString
   put "L7,110100101110110" & LF after tTmpString
   put "M0,101010000010010" & LF after tTmpString
   put "M1,101000100100101" & LF after tTmpString
   put "M2,101111001111100" & LF after tTmpString
   put "M3,101101101001011" & LF after tTmpString
   put "M4,100010111111001" & LF after tTmpString
   put "M5,100000011001110" & LF after tTmpString
   put "M6,100111110010111" & LF after tTmpString
   put "M7,100101010100000" & LF after tTmpString
   put "Q0,011010101011111" & LF after tTmpString
   put "Q1,011000001101000" & LF after tTmpString
   put "Q2,011111100110001" & LF after tTmpString
   put "Q3,011101000000110" & LF after tTmpString
   put "Q4,010010010110100" & LF after tTmpString
   put "Q5,010000110000011" & LF after tTmpString
   put "Q6,010111011011010" & LF after tTmpString
   put "Q7,010101111101101" & LF after tTmpString
   put "H0,001011010001001" & LF after tTmpString
   put "H1,001001110111110" & LF after tTmpString
   put "H2,001110011100111" & LF after tTmpString
   put "H3,001100111010000" & LF after tTmpString
   put "H4,000011101100010" & LF after tTmpString
   put "H5,000001001010101" & LF after tTmpString
   put "H6,000110100001100" & LF after tTmpString
   put "H7,000100000111011" after tTmpString
   put tTmpString into sFormatList
   put empty into tTmpString
   
   # version, databits (LSB -> MSB)
   put "7,001010010011111000" & LF after tTmpString
   put "8,001111011010000100" & LF after tTmpString
   put "9,100110010101100100" & LF after tTmpString
   put "10,110010110010010100" & LF after tTmpString
   put "11,011011111101110100" & LF after tTmpString
   put "12,010001101110001100" & LF after tTmpString
   put "13,111000100001101100" & LF after tTmpString
   put "14,101100000110011100" & LF after tTmpString
   put "15,000101001001111100" & LF after tTmpString
   put "16,000111101101000010" & LF after tTmpString
   put "17,101110100010100010" & LF after tTmpString
   put "18,111010000101010010" & LF after tTmpString
   put "19,010011001010110010" & LF after tTmpString
   put "20,011001011001001010" & LF after tTmpString
   put "21,110000010110101010" & LF after tTmpString
   put "22,100100110001011010" & LF after tTmpString
   put "23,001101111110111010" & LF after tTmpString
   put "24,001000110111000110" & LF after tTmpString
   put "25,100001111000100110" & LF after tTmpString
   put "26,110101011111010110" & LF after tTmpString
   put "27,011100010000110110" & LF after tTmpString
   put "28,010110000011001110" & LF after tTmpString
   put "29,111111001100101110" & LF after tTmpString
   put "30,101011101011011110" & LF after tTmpString
   put "31,000010100100111110" & LF after tTmpString
   put "32,101010111001000001" & LF after tTmpString
   put "33,000011110110100001" & LF after tTmpString
   put "34,010111010001010001" & LF after tTmpString
   put "35,111110011110110001" & LF after tTmpString
   put "36,110100001101001001" & LF after tTmpString
   put "37,011101000010101001" & LF after tTmpString
   put "38,001001100101011001" & LF after tTmpString
   put "39,100000101010111001" & LF after tTmpString
   put "40,100101100011000101" after tTmpString
   put tTmpString into sVersionList
   put empty into tTmpString
   
   # version, databits (MSB -> LSB)
   put "7,000111110010010100" & LF after tTmpString
   put "8,001000010110111100" & LF after tTmpString
   put "9,001001101010011001" & LF after tTmpString
   put "10,001010010011010011" & LF after tTmpString
   put "11,001011101111110110" & LF after tTmpString
   put "12,001100011101100010" & LF after tTmpString
   put "13,001101100001000111" & LF after tTmpString
   put "14,001110011000001101" & LF after tTmpString
   put "15,001111100100101000" & LF after tTmpString
   put "16,010000101101111000" & LF after tTmpString
   put "17,010001010001011101" & LF after tTmpString
   put "18,010010101000010111" & LF after tTmpString
   put "19,010011010100110010" & LF after tTmpString
   put "20,010100100110100110" & LF after tTmpString
   put "21,010101011010000011" & LF after tTmpString
   put "22,010110100011001001" & LF after tTmpString
   put "23,010111011111101100" & LF after tTmpString
   put "24,011000111011000100" & LF after tTmpString
   put "25,011001000111100001" & LF after tTmpString
   put "26,011010111110101011" & LF after tTmpString
   put "27,011011000010001110" & LF after tTmpString
   put "28,011100110000011010" & LF after tTmpString
   put "29,011101001100111111" & LF after tTmpString
   put "30,011110110101110101" & LF after tTmpString
   put "31,011111001001010000" & LF after tTmpString
   put "32,100000100111010101" & LF after tTmpString
   put "33,100001011011110000" & LF after tTmpString
   put "34,100010100010111010" & LF after tTmpString
   put "35,100011011110011111" & LF after tTmpString
   put "36,100100101100001011" & LF after tTmpString
   put "37,100101010000101110" & LF after tTmpString
   put "38,100110101001100100" & LF after tTmpString
   put "39,100111010101000001" & LF after tTmpString
   put "40,101000110001101001" after tTmpString
   put tTmpString into sVersionList2
   put empty into tTmpString
   
   put "AliceBlue,239,247,255" & LF after tTmpString
   put "AntiqueWhite,249,232,210" & LF after tTmpString
   put "AntiqueWhite1,254,237,214" & LF after tTmpString
   put "AntiqueWhite2,235,219,197" & LF after tTmpString
   put "AntiqueWhite3,200,185,166" & LF after tTmpString
   put "AntiqueWhite4,139,131,120" & LF after tTmpString
   put "AntiqueWhite4,139,131,120" & LF after tTmpString
   put "Aquamarine,67,183,186" & LF after tTmpString
   put "Aquamarine1,127,255,212" & LF after tTmpString
   put "Aquamarine2,118,238,198" & LF after tTmpString
   put "Aquamarine4,65,124,100" & LF after tTmpString
   put "Azure,239,255,255" & LF after tTmpString
   put "Azure2,222,236,236" & LF after tTmpString
   put "Azure3,188,199,199" & LF after tTmpString
   put "Azure4,122,125,125" & LF after tTmpString
   put "Beige,245,243,215" & LF after tTmpString
   put "Beige,245,243,215" & LF after tTmpString
   put "Bisque,255,228,196" & LF after tTmpString
   put "Bisque1,253,224,188" & LF after tTmpString
   put "Bisque2,234,208,174" & LF after tTmpString
   put "Bisque3,199,175,146" & LF after tTmpString
   put "Bisque4,129,110,89" & LF after tTmpString
   put "Black,0,0,0" & LF after tTmpString
   put "BlanchedAlmond,254,232,198" & LF after tTmpString
   put "Blue,0,0,255" & LF after tTmpString
   put "Blue1,21,53,255" & LF after tTmpString
   put "Blue2,21,49,236" & LF after tTmpString
   put "Blue3,21,40,199" & LF after tTmpString
   put "Blue4,21,27,126" & LF after tTmpString
   put "BlueViolet,138,43,226" & LF after tTmpString
   put "Brown,152,5,23" & LF after tTmpString
   put "Brown1,246,53,38" & LF after tTmpString
   put "Brown2,228,45,23" & LF after tTmpString
   put "Brown3,194,34,23" & LF after tTmpString
   put "Brown4,126,5,23" & LF after tTmpString
   put "Burlywood,216,175,121" & LF after tTmpString
   put "Burlywood1,252,206,142" & LF after tTmpString
   put "Burlywood2,234,190,131" & LF after tTmpString
   put "Burlywood3,198,160,109" & LF after tTmpString
   put "Burlywood4,128,99,65" & LF after tTmpString
   put "CadetBlue,87,134,147" & LF after tTmpString
   put "CadetBlue1,152,245,255" & LF after tTmpString
   put "CadetBlue2,142,226,236" & LF after tTmpString
   put "CadetBlue3,119,191,199" & LF after tTmpString
   put "CadetBlue4,76,120,126" & LF after tTmpString
   put "Chartreuse,127,255,0" & LF after tTmpString
   put "Chartreuse2,118,238,0" & LF after tTmpString
   put "Chartreuse3,102,205,0" & LF after tTmpString
   put "Chartreuse4,67,124,23" & LF after tTmpString
   put "Chocolate,200,90,23" & LF after tTmpString
   put "Chocolate1,248,114,23" & LF after tTmpString
   put "Chocolate2,229,103,23" & LF after tTmpString
   put "Chocolate3,195,86,23" & LF after tTmpString
   put "Coral,247,101,65" & LF after tTmpString
   put "Coral2,229,91,60" & LF after tTmpString
   put "Coral3,195,74,44" & LF after tTmpString
   put "Coral4,126,40,23" & LF after tTmpString
   put "CornflowerBlue,100,149,237" & LF after tTmpString
   put "CornSilk,255,247,215" & LF after tTmpString
   put "CornSilk2,236,229,198" & LF after tTmpString
   put "CornSilk3,200,194,167" & LF after tTmpString
   put "CornSilk4,129,122,104" & LF after tTmpString
   put "Cyan,0,255,255" & LF after tTmpString
   put "Cyan1,87,254,255" & LF after tTmpString
   put "Cyan2,80,235,236" & LF after tTmpString
   put "Cyan3,70,199,199" & LF after tTmpString
   put "Cyan4,48,125,126" & LF after tTmpString
   put "DarkBlue,0,0,139" & LF after tTmpString
   put "DarkCyan,0,139,139" & LF after tTmpString
   put "DarkGoldenrod,175,120,23" & LF after tTmpString
   put "DarkGoldenrod1,251,177,23" & LF after tTmpString
   put "DarkGoldenrod2,232,163,23" & LF after tTmpString
   put "DarkGoldenrod3,197,137,23" & LF after tTmpString
   put "DarkGoldenrod4,127,82,23" & LF after tTmpString
   put "DarkGray,169,169,169" & LF after tTmpString
   put "DarkGreen,0,100,0" & LF after tTmpString
   put "DarkKhaki,183,173,89" & LF after tTmpString
   put "DarkMagenta,139,0,139" & LF after tTmpString
   put "DarkOliveGreen,74,65,23" & LF after tTmpString
   put "DarkOliveGreen1,202,255,112" & LF after tTmpString
   put "DarkOliveGreen2,188,233,84" & LF after tTmpString
   put "DarkOliveGreen3,160,197,68" & LF after tTmpString
   put "DarkOliveGreen4,102,124,38" & LF after tTmpString
   put "DarkOrange,248,128,23" & LF after tTmpString
   put "DarkOrchid,125,27,126" & LF after tTmpString
   put "DarkOrchid1,176,65,255" & LF after tTmpString
   put "DarkOrchid2,162,59,236" & LF after tTmpString
   put "DarkOrchid3,139,49,199" & LF after tTmpString
   put "DarkOrchid4,104,34,139" & LF after tTmpString
   put "DarkRed,139,0,0" & LF after tTmpString
   put "DarkSeaGreen,139,179,129" & LF after tTmpString
   put "DarkSeaGreen1,193,255,193" & LF after tTmpString
   put "DarkSeaGreen2,180,238,180" & LF after tTmpString
   put "DarkSeaGreen3,153,198,142" & LF after tTmpString
   put "DarkSeaGreen4,105,139,105" & LF after tTmpString
   put "DarkSlateBlue,43,56,86" & LF after tTmpString
   put "DarkSlateGray,37,56,60" & LF after tTmpString
   put "DarkSlateGray1,151,255,255" & LF after tTmpString
   put "DarkSlateGray2,141,238,238" & LF after tTmpString
   put "DarkSlateGray3,120,199,199" & LF after tTmpString
   put "DarkSlateGray4,76,125,126" & LF after tTmpString
   put "DarkTurquoise,59,156,156" & LF after tTmpString
   put "DarkViolet,132,45,206" & LF after tTmpString
   put "DeepPink,245,40,135" & LF after tTmpString
   put "DeepPink1,255,20,147" & LF after tTmpString
   put "DeepPink2,228,40,124" & LF after tTmpString
   put "DeepPink3,193,34,103" & LF after tTmpString
   put "DeepPink4,125,5,63" & LF after tTmpString
   put "DeepSkyBlue,0,191,255" & LF after tTmpString
   put "DeepSkyBlue1,59,185,255" & LF after tTmpString
   put "DeepSkyBlue2,0,178,238" & LF after tTmpString
   put "DeepSkyBlue3,0,154,205" & LF after tTmpString
   put "DeepSkyBlue4,0,104,139" & LF after tTmpString
   put "DimGray,70,62,65" & LF after tTmpString
   put "DodgerBlue,21,137,255" & LF after tTmpString
   put "DodgerBlue1,30,144,255" & LF after tTmpString
   put "DodgerBlue2,28,134,238" & LF after tTmpString
   put "DodgerBlue3,24,116,205" & LF after tTmpString
   put "DodgerBlue4,16,78,139" & LF after tTmpString
   put "Firebrick,128,5,23" & LF after tTmpString
   put "Firebrick1,246,40,23" & LF after tTmpString
   put "Firebrick2,228,34,23" & LF after tTmpString
   put "FloralWhite,255,249,238" & LF after tTmpString
   put "ForestGreen,34,139,34" & LF after tTmpString
   put "Gainsboro,216,217,215" & LF after tTmpString
   put "GhostWhite,247,247,255" & LF after tTmpString
   put "Gold,212,160,23" & LF after tTmpString
   put "Gold1,253,208,23" & LF after tTmpString
   put "Gold2,234,193,23" & LF after tTmpString
   put "Gold3,199,163,23" & LF after tTmpString
   put "Gold4,128,101,23" & LF after tTmpString
   put "Goldenrod,218,165,32" & LF after tTmpString
   put "Goldenrod1,251,185,23" & LF after tTmpString
   put "Goldenrod2,233,171,23" & LF after tTmpString
   put "Goldenrod3,198,142,23" & LF after tTmpString
   put "Goldenrod4,128,88,23" & LF after tTmpString
   put "Gray,190,190,190" & LF after tTmpString
   put "Gray18,37,5,23" & LF after tTmpString
   put "Gray21,43,27,23" & LF after tTmpString
   put "Gray23,48,34,23" & LF after tTmpString
   put "Gray24,48,34,38" & LF after tTmpString
   put "Gray25,52,40,38" & LF after tTmpString
   put "Gray26,52,40,44" & LF after tTmpString
   put "Gray27,56,45,44" & LF after tTmpString
   put "Gray28,59,49,49" & LF after tTmpString
   put "Gray29,62,53,53" & LF after tTmpString
   put "Gray30,65,56,57" & LF after tTmpString
   put "Gray31,65,56,60" & LF after tTmpString
   put "Gray32,70,62,63" & LF after tTmpString
   put "Gray34,74,67,68" & LF after tTmpString
   put "Gray35,76,70,70" & LF after tTmpString
   put "Gray36,78,72,72" & LF after tTmpString
   put "Gray37,80,74,75" & LF after tTmpString
   put "Gray38,84,78,79" & LF after tTmpString
   put "Gray39,86,80,81" & LF after tTmpString
   put "Gray4,10,10,10" & LF after tTmpString
   put "Gray40,102,102,102" & LF after tTmpString
   put "Gray41,105,105,105" & LF after tTmpString
   put "Gray42,107,107,107" & LF after tTmpString
   put "Gray43,110,110,110" & LF after tTmpString
   put "Gray44,100,96,96" & LF after tTmpString
   put "Gray45,102,99,98" & LF after tTmpString
   put "Gray46,105,101,101" & LF after tTmpString
   put "Gray47,109,105,104" & LF after tTmpString
   put "Gray48,110,106,107" & LF after tTmpString
   put "Gray49,114,110,109" & LF after tTmpString
   put "Gray5,13,13,13" & LF after tTmpString
   put "Gray50,116,113,112" & LF after tTmpString
   put "Gray51,120,116,115" & LF after tTmpString
   put "Gray52,122,119,119" & LF after tTmpString
   put "Gray53,124,121,121" & LF after tTmpString
   put "Gray54,128,125,124" & LF after tTmpString
   put "Gray55,130,128,126" & LF after tTmpString
   put "Gray56,133,131,129" & LF after tTmpString
   put "Gray57,135,133,131" & LF after tTmpString
   put "Gray58,139,137,135" & LF after tTmpString
   put "Gray59,141,139,137" & LF after tTmpString
   put "Gray6,15,15,15" & LF after tTmpString
   put "Gray60,143,142,141" & LF after tTmpString
   put "Gray61,147,145,144" & LF after tTmpString
   put "Gray62,149,148,146" & LF after tTmpString
   put "Gray63,153,151,149" & LF after tTmpString
   put "Gray64,154,153,152" & LF after tTmpString
   put "Gray65,158,156,155" & LF after tTmpString
   put "Gray67,163,162,160" & LF after tTmpString
   put "Gray68,165,164,163" & LF after tTmpString
   put "Gray69,169,168,166" & LF after tTmpString
   put "Gray7,18,18,18" & LF after tTmpString
   put "Gray70,172,171,169" & LF after tTmpString
   put "Gray71,174,173,172" & LF after tTmpString
   put "Gray72,177,177,175" & LF after tTmpString
   put "Gray73,179,179,177" & LF after tTmpString
   put "Gray74,183,182,180" & LF after tTmpString
   put "Gray75,185,184,182" & LF after tTmpString
   put "Gray76,188,187,186" & LF after tTmpString
   put "Gray77,190,190,188" & LF after tTmpString
   put "Gray78,193,193,191" & LF after tTmpString
   put "Gray79,195,196,194" & LF after tTmpString
   put "Gray8,20,20,20" & LF after tTmpString
   put "Gray80,199,199,197" & LF after tTmpString
   put "Gray81,202,202,201" & LF after tTmpString
   put "Gray82,204,204,203" & LF after tTmpString
   put "Gray83,208,207,207" & LF after tTmpString
   put "Gray84,210,210,209" & LF after tTmpString
   put "Gray85,213,213,212" & LF after tTmpString
   put "Gray86,215,215,215" & LF after tTmpString
   put "Gray87,219,219,217" & LF after tTmpString
   put "Gray88,221,221,220" & LF after tTmpString
   put "Gray89,224,224,224" & LF after tTmpString
   put "Gray90,226,227,225" & LF after tTmpString
   put "Gray91,229,230,228" & LF after tTmpString
   put "Gray92,232,233,232" & LF after tTmpString
   put "Gray93,235,235,234" & LF after tTmpString
   put "Gray94,238,238,238" & LF after tTmpString
   put "Gray95,240,241,240" & LF after tTmpString
   put "Gray97,246,246,245" & LF after tTmpString
   put "Gray98,249,249,250" & LF after tTmpString
   put "Gray99,251,251,251" & LF after tTmpString
   put "Green,0,255,0" & LF after tTmpString
   put "Green1,95,251,23" & LF after tTmpString
   put "Green2,0,238,0" & LF after tTmpString
   put "Green3,0,205,0" & LF after tTmpString
   put "Green4,0,139,0" & LF after tTmpString
   put "GreenYellow,173,255,47" & LF after tTmpString
   put "Honeydew,240,254,238" & LF after tTmpString
   put "Honeydew1,240,255,240" & LF after tTmpString
   put "Honeydew2,222,235,220" & LF after tTmpString
   put "Honeydew3,188,199,185" & LF after tTmpString
   put "Honeydew4,122,125,116" & LF after tTmpString
   put "HotPink,246,96,171" & LF after tTmpString
   put "HotPink1,246,101,171" & LF after tTmpString
   put "HotPink2,228,94,157" & LF after tTmpString
   put "HotPink3,194,82,131" & LF after tTmpString
   put "HotPink4,125,34,82" & LF after tTmpString
   put "IndianRed,205,92,92" & LF after tTmpString
   put "IndianRed1,247,93,89" & LF after tTmpString
   put "IndianRed2,229,84,81" & LF after tTmpString
   put "IndianRed3,194,70,65" & LF after tTmpString
   put "Ivory,255,255,238" & LF after tTmpString
   put "Ivory2,236,236,220" & LF after tTmpString
   put "Ivory3,201,199,185" & LF after tTmpString
   put "Ivory4,129,125,116" & LF after tTmpString
   put "Khaki,173,169,110" & LF after tTmpString
   put "Khaki1,255,243,128" & LF after tTmpString
   put "Khaki2,237,226,117" & LF after tTmpString
   put "Khaki3,201,190,98" & LF after tTmpString
   put "Khaki4,130,120,57" & LF after tTmpString
   put "Lavender,227,228,250" & LF after tTmpString
   put "LavenderBlush,253,238,244" & LF after tTmpString
   put "LavenderBlush1,255,240,245" & LF after tTmpString
   put "LavenderBlush2,235,221,226" & LF after tTmpString
   put "LavenderBlush3,200,187,190" & LF after tTmpString
   put "LavenderBlush4,129,118,121" & LF after tTmpString
   put "LawnGreen,124,252,0" & LF after tTmpString
   put "LemonChiffon,255,248,198" & LF after tTmpString
   put "LemonChiffon1,255,250,205" & LF after tTmpString
   put "LemonChiffon2,238,233,191" & LF after tTmpString
   put "LemonChiffon3,205,201,165" & LF after tTmpString
   put "LemonChiffon4,139,137,112" & LF after tTmpString
   put "LightBlue,173,216,230" & LF after tTmpString
   put "LightBlue1,189,237,255" & LF after tTmpString
   put "LightBlue2,175,220,236" & LF after tTmpString
   put "LightBlue3,149,185,199" & LF after tTmpString
   put "LightBlue4,104,131,139" & LF after tTmpString
   put "LightCoral,231,116,113" & LF after tTmpString
   put "LightCyan,224,255,255" & LF after tTmpString
   put "LightCyan2,207,236,236" & LF after tTmpString
   put "LightCyan3,175,199,199" & LF after tTmpString
   put "LightCyan4,113,125,125" & LF after tTmpString
   put "LightGoldenrod,236,216,114" & LF after tTmpString
   put "LightGoldenrod1,255,232,124" & LF after tTmpString
   put "LightGoldenrod2,236,214,114" & LF after tTmpString
   put "LightGoldenrod3,200,181,96" & LF after tTmpString
   put "LightGoldenrod4,129,115,57" & LF after tTmpString
   put "LightGoldenrodYellow,250,248,204" & LF after tTmpString
   put "LightGray,160,159,157" & LF after tTmpString
   put "LightGreen,144,238,144" & LF after tTmpString
   put "LightPink,250,175,186" & LF after tTmpString
   put "LightPink1,249,167,176" & LF after tTmpString
   put "LightPink2,231,153,163" & LF after tTmpString
   put "LightPink3,196,129,137" & LF after tTmpString
   put "LightPink4,127,78,82" & LF after tTmpString
   put "LightSalmon,249,150,107" & LF after tTmpString
   put "LightSalmon1,255,160,122" & LF after tTmpString
   put "LightSalmon2,231,138,97" & LF after tTmpString
   put "LightSalmon3,196,116,81" & LF after tTmpString
   put "LightSalmon4,127,70,44" & LF after tTmpString
   put "LightSeaGreen,32,178,170" & LF after tTmpString
   put "LightSkyBlue,130,202,250" & LF after tTmpString
   put "LightSkyBlue1,173,223,255" & LF after tTmpString
   put "LightSkyBlue2,160,207,236" & LF after tTmpString
   put "LightSkyBlue3,135,175,199" & LF after tTmpString
   put "LightSkyBlue4,86,109,126" & LF after tTmpString
   put "LightSlateBlue,115,106,255" & LF after tTmpString
   put "LightSlateGray,109,123,141" & LF after tTmpString
   put "LightSteelBlue,114,143,206" & LF after tTmpString
   put "LightSteelBlue1,198,222,255" & LF after tTmpString
   put "LightSteelBlue2,183,206,236" & LF after tTmpString
   put "LightSteelBlue3,154,173,199" & LF after tTmpString
   put "LightSteelBlue4,100,109,126" & LF after tTmpString
   put "LightYellow,255,254,220" & LF after tTmpString
   put "LightYellow1,255,255,224" & LF after tTmpString
   put "LightYellow2,237,235,203" & LF after tTmpString
   put "LightYellow3,201,199,170" & LF after tTmpString
   put "LightYellow4,130,125,107" & LF after tTmpString
   put "LimeGreen,50,205,50" & LF after tTmpString
   put "Linen,249,238,226" & LF after tTmpString
   put "Magenta,244,62,255" & LF after tTmpString
   put "Magenta1,255,0,255" & LF after tTmpString
   put "Magenta2,226,56,236" & LF after tTmpString
   put "Magenta3,192,49,199" & LF after tTmpString
   put "Maroon,129,5,65" & LF after tTmpString
   put "Maroon1,245,53,170" & LF after tTmpString
   put "Maroon2,227,49,157" & LF after tTmpString
   put "Maroon3,193,34,131" & LF after tTmpString
   put "Maroon4,125,5,82" & LF after tTmpString
   put "MediumAquamarine,102,205,170" & LF after tTmpString
   put "MediumBlue,0,0,205" & LF after tTmpString
   put "MediumForestGreen,52,114,53" & LF after tTmpString
   put "MediumGoldenrod,204,185,84" & LF after tTmpString
   put "MediumOrchid,176,72,181" & LF after tTmpString
   put "MediumOrchid1,212,98,255" & LF after tTmpString
   put "MediumOrchid2,196,90,236" & LF after tTmpString
   put "MediumOrchid3,167,74,199" & LF after tTmpString
   put "MediumOrchid4,106,40,126" & LF after tTmpString
   put "MediumPurple,132,103,215" & LF after tTmpString
   put "MediumPurple1,171,130,255" & LF after tTmpString
   put "MediumPurple2,159,121,238" & LF after tTmpString
   put "MediumPurple3,137,104,205" & LF after tTmpString
   put "MediumPurple4,93,71,139" & LF after tTmpString
   put "MediumSeaGreen,48,103,84" & LF after tTmpString
   put "MediumSlateBlue,123,104,238" & LF after tTmpString
   put "MediumSpringGreen,0,250,154" & LF after tTmpString
   put "MediumTurquoise,72,204,205" & LF after tTmpString
   put "MediumVioletRed,199,21,133" & LF after tTmpString
   put "MidnightBlue,21,27,84" & LF after tTmpString
   put "MintCream,245,255,249" & LF after tTmpString
   put "MistyRose,253,225,221" & LF after tTmpString
   put "MistyRose1,255,228,225" & LF after tTmpString
   put "MistyRose2,234,208,204" & LF after tTmpString
   put "MistyRose3,198,175,172" & LF after tTmpString
   put "MistyRose4,128,111,108" & LF after tTmpString
   put "Moccasin,253,224,172" & LF after tTmpString
   put "NavajoWhite,253,218,163" & LF after tTmpString
   put "NavajoWhite1,255,222,173" & LF after tTmpString
   put "NavajoWhite2,234,201,149" & LF after tTmpString
   put "NavajoWhite3,199,170,125" & LF after tTmpString
   put "NavajoWhite4,128,106,75" & LF after tTmpString
   put "Navy,0,0,128" & LF after tTmpString
   put "OldLace,252,243,226" & LF after tTmpString
   put "OliveDrab,101,128,23" & LF after tTmpString
   put "OliveDrab1,192,255,62" & LF after tTmpString
   put "OliveDrab2,179,238,58" & LF after tTmpString
   put "OliveDrab3,153,197,23" & LF after tTmpString
   put "OliveDrab4,105,139,34" & LF after tTmpString
   put "Orange,248,122,23" & LF after tTmpString
   put "Orange1,250,155,23" & LF after tTmpString
   put "Orange2,231,142,23" & LF after tTmpString
   put "Orange3,197,119,23" & LF after tTmpString
   put "Orange4,127,72,23" & LF after tTmpString
   put "OrangeRed,246,56,23" & LF after tTmpString
   put "OrangeRed1,255,69,0" & LF after tTmpString
   put "OrangeRed2,228,49,23" & LF after tTmpString
   put "OrangeRed3,194,40,23" & LF after tTmpString
   put "Orchid,218,112,214" & LF after tTmpString
   put "Orchid1,246,125,250" & LF after tTmpString
   put "Orchid2,228,115,231" & LF after tTmpString
   put "Orchid3,193,96,195" & LF after tTmpString
   put "Orchid4,125,56,124" & LF after tTmpString
   put "PaleGoldenrod,237,228,158" & LF after tTmpString
   put "PaleGreen,121,216,103" & LF after tTmpString
   put "PaleGreen1,154,255,154" & LF after tTmpString
   put "PaleGreen3,124,205,124" & LF after tTmpString
   put "PaleGreen4,78,124,65" & LF after tTmpString
   put "PaleTurquoise,174,235,236" & LF after tTmpString
   put "PaleTurquoise1,187,255,255" & LF after tTmpString
   put "PaleTurquoise2,173,235,236" & LF after tTmpString
   put "PaleTurquoise3,146,199,199" & LF after tTmpString
   put "PaleTurquoise4,102,139,139" & LF after tTmpString
   put "PaleVioletRed,209,101,135" & LF after tTmpString
   put "PaleVioletRed1,247,120,161" & LF after tTmpString
   put "PaleVioletRed2,229,110,148" & LF after tTmpString
   put "PaleVioletRed3,194,90,124" & LF after tTmpString
   put "PaleVioletRed4,126,53,77" & LF after tTmpString
   put "PapayaWhip,254,236,207" & LF after tTmpString
   put "PeachPuff,252,213,176" & LF after tTmpString
   put "PeachPuff1,255,218,185" & LF after tTmpString
   put "PeachPuff2,234,197,163" & LF after tTmpString
   put "PeachPuff3,198,166,136" & LF after tTmpString
   put "PeachPuff4,128,103,82" & LF after tTmpString
   put "Peru,197,119,38" & LF after tTmpString
   put "Pink,250,175,190" & LF after tTmpString
   put "Pink1,255,181,197" & LF after tTmpString
   put "Pink2,231,161,176" & LF after tTmpString
   put "Pink3,196,135,147" & LF after tTmpString
   put "Pink4,127,82,93" & LF after tTmpString
   put "Plum,185,59,143" & LF after tTmpString
   put "Plum1,249,183,255" & LF after tTmpString
   put "Plum2,230,169,236" & LF after tTmpString
   put "Plum3,195,142,199" & LF after tTmpString
   put "Plum4,126,88,126" & LF after tTmpString
   put "PowderBlue,173,220,227" & LF after tTmpString
   put "Purple,142,53,239" & LF after tTmpString
   put "Purple1,137,59,255" & LF after tTmpString
   put "Purple2,127,56,236" & LF after tTmpString
   put "Purple3,108,45,199" & LF after tTmpString
   put "Purple4,70,27,126" & LF after tTmpString
   put "Red,255,0,0" & LF after tTmpString
   put "Red1,246,34,23" & LF after tTmpString
   put "Red2,228,27,23" & LF after tTmpString
   put "Red3,193,27,23" & LF after tTmpString
   put "RosyBrown,179,132,129" & LF after tTmpString
   put "RosyBrown1,251,187,185" & LF after tTmpString
   put "RosyBrown2,232,173,170" & LF after tTmpString
   put "RosyBrown3,197,144,142" & LF after tTmpString
   put "RosyBrown4,127,90,88" & LF after tTmpString
   put "RoyalBlue,43,96,222" & LF after tTmpString
   put "RoyalBlue1,48,110,255" & LF after tTmpString
   put "RoyalBlue2,43,101,236" & LF after tTmpString
   put "RoyalBlue3,37,84,199" & LF after tTmpString
   put "RoyalBlue4,21,49,126" & LF after tTmpString
   put "SaddleBrown,126,49,23" & LF after tTmpString
   put "Salmon,225,139,107" & LF after tTmpString
   put "Salmon1,248,129,88" & LF after tTmpString
   put "Salmon2,230,116,81" & LF after tTmpString
   put "Salmon3,195,98,65" & LF after tTmpString
   put "Salmon4,126,56,23" & LF after tTmpString
   put "SandyBrown,238,154,77" & LF after tTmpString
   put "SeaGreen,46,139,87" & LF after tTmpString
   put "SeaGreen1,106,251,146" & LF after tTmpString
   put "SeaGreen2,100,233,134" & LF after tTmpString
   put "SeaGreen3,67,205,128" & LF after tTmpString
   put "Seashell,254,243,235" & LF after tTmpString
   put "Seashell1,255,245,238" & LF after tTmpString
   put "Seashell2,238,229,222" & LF after tTmpString
   put "Seashell3,205,197,191" & LF after tTmpString
   put "Seashell4,139,134,130" & LF after tTmpString
   put "Sienna,160,82,45" & LF after tTmpString
   put "Sienna1,248,116,49" & LF after tTmpString
   put "Sienna2,230,108,44" & LF after tTmpString
   put "Sienna3,195,88,23" & LF after tTmpString
   put "Sienna4,126,53,23" & LF after tTmpString
   put "SkyBlue,102,152,255" & LF after tTmpString
   put "SkyBlue1,130,202,255" & LF after tTmpString
   put "SkyBlue2,121,186,236" & LF after tTmpString
   put "SkyBlue3,101,158,199" & LF after tTmpString
   put "SkyBlue4,65,98,126" & LF after tTmpString
   put "SlateBlue,106,90,205" & LF after tTmpString
   put "SlateBlue1,115,105,255" & LF after tTmpString
   put "SlateBlue2,105,96,236" & LF after tTmpString
   put "SlateBlue3,105,89,205" & LF after tTmpString
   put "SlateBlue4,52,45,126" & LF after tTmpString
   put "SlateGray,101,115,131" & LF after tTmpString
   put "SlateGray1,194,223,255" & LF after tTmpString
   put "SlateGray2,180,207,236" & LF after tTmpString
   put "SlateGray3,152,175,199" & LF after tTmpString
   put "SlateGray4,108,123,139" & LF after tTmpString
   put "Snow,255,249,250" & LF after tTmpString
   put "Snow1,255,250,250" & LF after tTmpString
   put "Snow2,236,231,230" & LF after tTmpString
   put "Snow3,200,196,194" & LF after tTmpString
   put "Snow4,129,124,123" & LF after tTmpString
   put "SpringGreen,0,255,127" & LF after tTmpString
   put "SpringGreen1,94,251,110" & LF after tTmpString
   put "SpringGreen2,0,238,118" & LF after tTmpString
   put "SpringGreen3,0,205,102" & LF after tTmpString
   put "SpringGreen4,0,139,69" & LF after tTmpString
   put "SteelBlue,70,130,180" & LF after tTmpString
   put "SteelBlue1,92,179,255" & LF after tTmpString
   put "SteelBlue2,86,165,236" & LF after tTmpString
   put "SteelBlue3,72,138,199" & LF after tTmpString
   put "SteelBlue4,43,84,126" & LF after tTmpString
   put "Tan,210,180,140" & LF after tTmpString
   put "Tan1,250,155,60" & LF after tTmpString
   put "Tan2,231,142,53" & LF after tTmpString
   put "Thistle,210,185,211" & LF after tTmpString
   put "Thistle1,252,223,255" & LF after tTmpString
   put "Thistle2,233,207,236" & LF after tTmpString
   put "Thistle3,198,174,199" & LF after tTmpString
   put "Thistle4,128,109,126" & LF after tTmpString
   put "Tomato,247,84,49" & LF after tTmpString
   put "Tomato1,255,99,71" & LF after tTmpString
   put "Tomato2,229,76,44" & LF after tTmpString
   put "Tomato3,194,62,23" & LF after tTmpString
   put "Tomato4,126,34,23" & LF after tTmpString
   put "Transparent,21,5,23" & LF after tTmpString
   put "Turquoise,64,224,208" & LF after tTmpString
   put "Turquoise1,0,245,255" & LF after tTmpString
   put "Turquoise2,0,229,238" & LF after tTmpString
   put "Turquoise3,0,197,205" & LF after tTmpString
   put "Turquoise4,0,134,139" & LF after tTmpString
   put "Violet,141,56,201" & LF after tTmpString
   put "VioletRed,208,32,144" & LF after tTmpString
   put "VioletRed1,246,53,138" & LF after tTmpString
   put "VioletRed2,228,49,127" & LF after tTmpString
   put "VioletRed3,193,40,105" & LF after tTmpString
   put "VioletRed4,125,5,65" & LF after tTmpString
   put "Wheat,243,218,169" & LF after tTmpString
   put "Wheat1,254,228,177" & LF after tTmpString
   put "Wheat2,235,211,163" & LF after tTmpString
   put "Wheat3,200,177,137" & LF after tTmpString
   put "Wheat4,129,111,84" & LF after tTmpString
   put "White,255,255,255" & LF after tTmpString
   put "WhiteSmoke,244,244,243" & LF after tTmpString
   put "Yellow,255,255,0" & LF after tTmpString
   put "Yellow2,238,233,23" & LF after tTmpString
   put "Yellow3,202,197,23" & LF after tTmpString
   put "Yellow4,130,124,23" & LF after tTmpString
   put "YellowGreen,154,205,50" after tTmpString
   put tTmpString into sColorNames
   put empty into tTmpString
   
end __Initialize


private command _checkVersionInfo
   repeat with tV = 7 to 40
      local tLine1, tLine2, tBits1, tBits2, tLen1, tLen2
      put lineOffset(LF & tV, LF & sVersionList) into tLine1
      put lineOffset(LF & tV, LF & sVersionList2) into tLine2
      put item 2 of line tLine1 of sVersionList into tBits1
      put item 2 of line tLine2 of sVersionList2 into tBits2
      put length(tBits1) into tLen1
      put length(tBits2) into tLen2
      if tLen1 <> tLen2 then
         answer "Version" && tV && "is different length"
         exit to top
      end if
      repeat with tIndex = 1 to tLen1
         if char tIndex of tBits1 <> char tLen2 - tIndex + 1 of tBits2 then
            answer "Version" && tV && "has different data"
            exit to top
         end if
      end repeat
   end repeat
   answer "All versions checked"
end _checkVersionInfo


private function _size pVersion
   # QR code size not including quiet zone
   return 21 + (pVersion - 1) * 4
end _size


private command _draw pX, pY, pColor, pMode
   local tVal, tPos
   if (pMode bitAnd 1) = 0 then
      # update image data
      if pColor = 1 then put sWhitePixel into tVal else put sBlackPixel into tVal
      put (pX + sQZSize - 1) * 4 + 1 + (pY - 1 + sQZSize) * 4 * sImgSize into tPos
      put tVal into char tPos to tPos + 3 of sImgData
   end if
   
   # update symbol map
   put 1 into char pX + (pY - 1) * sQRSize of sSymbolMap
   
   # update data map
   if pColor = 1 then put 0 into tVal else put 1 into tVal
   put tVal into char pX + (pY - 1) * sQRSize of sDataMap
end _draw


private function _format pData, pLen
   local tPad
   put pLen - length(pData) into tPad
   if tPad > 0 then
      repeat tPad
         put "0" before pData
      end repeat
   end if
   return pData
end _format


private function _alpha pInt
   # lookup integer in the GF256 log antilog table and return exponent
   return item 3 of line pInt of sGF256List
end _alpha


private function _int pAlpha
   # lookup exponent in the GF256 log antilog table and return integer
   if pAlpha = 0 then return 1
   return item 2 of line pAlpha of sGF256List
end _int


private function _byteValues pData
   local tVals
   repeat for each char tChar in pData
      put charToNum(tChar) & comma after tVals
   end repeat
   delete char -1 of tVals
   return tVals
end _byteValues


private function _errorCodes pByteStream, pNumErrorCodes
   local tNumDataCodes, tTotalCodes, tRefExp, tExp, tErrorCodeA, tMsgPolyA
   put length(pByteStream) into tNumDataCodes
   put tNumDataCodes + pNumErrorCodes into tTotalCodes
   put tTotalCodes - 1 into tRefExp
   put tRefExp into tExp
   put empty into tErrorCodeA
   # store the message polynomial coefficients
   put empty into tMsgPolyA
   repeat with i = 1 to tNumDataCodes
      put charToNum(char i of pByteStream) into tErrorCodeA[tExp]
      subtract 1 from tExp
   end repeat
   # zero fill the remaining coefficients
   repeat with i = tExp to 0 step -1
      put 0 into tErrorCodeA[i]
   end repeat
   # protect from infinite loop
   local tSanity
   put 0 into tSanity
   # get the generator polynomial exponents
   local tLine
   put lineOffset(LF & pNumErrorCodes, LF & sGenPolyAlphaExpList) into tLine
   
   local tGenData
   put line tLine of sGenPolyAlphaExpList into tGenData
   
   local tGenExp
   put item 2 to -1 of tGenData into tGenExp
   
   local tGenExpCount
   put the num of items in tGenExp into tGenExpCount
   repeat until the num of lines in the keys of tErrorCodeA = pNumErrorCodes or tSanity > 1000
      add 1 to tSanity
      # first coefficient alpha value: to add to all the generator alpha values
      local tRefAlpha
      put _alpha(tErrorCodeA[tRefExp]) into tRefAlpha
      repeat with i = 1 to tGenExpCount
         local tECIndex, tGenAlpha
         put tRefExp - i + 1 into tECIndex
         put item i of tGenExp into tGenAlpha
         # calculate the new coefficient value
         put _int((tRefAlpha + tGenAlpha) mod 255) bitXOr tErrorCodeA[tECIndex] into tErrorCodeA[tECIndex]
      end repeat
      # remove leading zero coefficients
      repeat with i = tRefExp to 0 step -1
         if tErrorCodeA[i] = 0 then
            delete variable tErrorCodeA[i]
            subtract 1 from tRefExp
         else
            exit repeat
         end if
      end repeat
   end repeat
   
   # return the error codes as a binary stream
   local tKeys
   put the keys of tErrorCodeA into tKeys
   sort lines of tKeys descending numeric
   local tErrorBytes
   repeat for each line tKey in tKeys
      put numToChar(tErrorCodeA[tKey]) after tErrorBytes
   end repeat
   return tErrorBytes
end _errorCodes

/**

Create a QR code

Example:
qrCreate "MyImage", "QR data", "M", 3

Example: 
--creating QR Code with ECC level M, Size 3, Mask 7 and version 14
qrCreate "MyImage", "QR data", "M", 3, 7, 14, 14

Example:
-- example how to use qrCreate with LiveCode server
qrCreate "!", tData, tECC, tSize
put the result into tData
if tData begins with "Error" then
   put tData
else
   # line 1 of the result contains some info
   # the remaining lines contain the png image data
   put line 1 of tData into tInfo
   delete line 1 of tData

   put "&lt;b&gt;Qr code info&lt;/b&gt;&lt;br/&gt;" & LF
   put "Version: " & item 1 of tInfo & "&lt;br/&gt;" & LF
   put "ECC: " & item 2 of tInfo & "&lt;br/&gt;" & LF
   put "Mode: " & item 3 of tInfo & "&lt;br/&gt;" & LF
   put "Mask: " & item 4 of tInfo & "&lt;br/&gt;" & LF

   # show the image
   put base64Encode(tData) into tData
   replace LF with empty in tData
   put "&lt;img src='data:image/png;base64," & tData & "'/&gt;&lt;br/&gt;" & LF
end if


Description:
Set the text of an image to a QR code or get the PMG data of the QR code image.

Parameters:
pImg:
The name of an image object on the card, a filename or the special code "!" which
will return the image data in the result.

pData:
The data to be encoded.

pECC (enum):
The error correction level

- "L" : 7% error correction
- "M" : 15% error correction
- "Q" : 25% error correction
- "H" : 30% error correction

pSize (integer):
Module size you can make the code larger/smaller.  You can also just
rescale the image after the code has been created.

pMask (integer):
An integer value between 0 and 7. When encoding a QR code, there are
eight mask patterns that you can use to change the outputted matrix.
Each mask pattern changes the bits according to their coordinates in
the QR matrix. The purpose of a mask pattern is to make the QR code
easier for a QR scanner to read.

pMinVersion:
An integer value between 1 and 40. It must be lower or equal to pMaxVersion.
If pMinVersion is not specified then the default value of 1 is used.

pMaxVersion:
An integer value between 1 and 40. It must be higher or equal to pMinVersion.
If pMaxVersion is not specified then the default value of 40 is used.

The result:
If successful, the first line of the result will be the following values: 
- QR version
- ECC
- Mode
- Mask
- Image size with quiet zone (based on module size 1)

If there was an error the result will contain "Error:" and then an error
description.

If the special character '!' is used for pImg, then the result also
contains the raw output of a png image.  You can use this data, for example,
with LiveCode server to generate QR codes on web pages.

*/

command qrCreate pImg, pData, pECC, pSize, pMask, pMinVersion, pMaxVersion
   lock screen
   
   try
      
      # check target image object exists
      local tPNG
      if there is an img pImg then
         put the long id of img pImg into pImg
         put false into tPNG
      else
         put not exists(pImg) into tPNG
      end if
      
      # check ECC
      if not(pECC is among the items of "L,M,Q,H") then
         throw "ECC must be L, M, Q or H"
      end if
      put pECC into sECC
      
      # min version, max version, numeric mode, alpha mode, 8-bit mode, Kanji mode
      if matchText(pData, "^[[:digit:]]+$") = true then
         put sModeNumeric into sMode
      else if matchText(pData, "^[" & sAlphaChars & "]+$") = true then
         put sModeAlpha into sMode
      else
         put sModeByte into sMode
      end if
      
      # parameters OK - start timimg
      local tMillisecs
      put the millisecs into tMillisecs
      
      local tDataLength
      put length(pData) into tDataLength
      
      # calculate minimum version required
      local tMinVersion
      put 0 into tMinVersion
      
      if pMinVersion is empty then 
         put 1 into pMinVersion
      end if
      
      if pMaxVersion is empty then 
         put 40 into pMaxVersion
      end if
      
      if pMaxVersion > 40 or pMaxVersion < 1 or pMinVersion < 1 or pMinVersion > 40 then
         throw "allowed values for pMinVersion to pMaxVersion are 1 to 40"
      end if
      
      if pMinVersion > pMaxVersion then 
         throw "pMinversion must be lower or equal to pMaxVersion"
      end if
     
      repeat with tVersion = pMinVersion to pMaxVersion
         local tCCIBits, tBitStreamLength, tErrorWords, tTotalWords, tBitStreamMax, tRemBits
         put 0 into tCCIBits -- character count indicator
         put 0 into tBitStreamLength
         repeat for each line tCCIData in sCCIList
            if tVersion >= item 1 of tCCIData and tVersion <= item 2 of tCCIData then
               # total data codewords and error codewords
               put item 2 of line tVersion of sCapacityList into tTotalWords
               put item itemOffset(sECC, "L,M,Q,H") + 3 of line tVersion of sCapacityList into tErrorWords
               # maximum bit stream length for version & EEC
               put (tTotalWords - tErrorWords) * 8 into tBitStreamMax
               # use CCI for correct mode
               switch sMode
                  case sModeNumeric
                     put item 3 of tCCIData into tCCIBits
                     put item ((tDataLength mod 3) + 1) of "0,4,7" into tRemBits
                     put 4 + tCCIBits + (10 * (tDataLength div 3)) + tRemBits into tBitStreamLength
                     break
                     
                  case sModeAlpha
                     put item 4 of tCCIData into tCCIBits
                     put 4 + tCCIBits + (11 * (tDataLength div 2)) + (6 * (tDataLength mod 2)) into tBitStreamLength
                     break
                     
                  case sModeByte
                     put item 5 of tCCIData into tCCIBits
                     put 4 + tCCIBits + (8 * tDataLength) into tBitStreamLength
                     break
               end switch
               # check if data fits in current version 
               if tBitStreamLength > 0 and tBitStreamLength <= tBitStreamMax then
                  put tVersion into tMinVersion
                  exit repeat
               end if
            end if
         end repeat
         # check if we have already found the minimum version
         if tMinVersion > 0 then
            exit repeat
         end if
      end repeat
      
      if tMinVersion = 0 then
         # too much data even for version 40
         throw "Data too large for QRCode"
      end if
      put tMinVersion into sVersion
      
      # create the data bit stream
      switch sMode
         case sModeNumeric
            local tData, tLength, tBits
            # add numeric mode indicator and CCI to bit stream
            put "0001" & _format(baseConvert(tDataLength, 10, 2), tCCIBits) into sBitStream
            repeat until pData = empty
               put char 1 to 3 of pData into tData
               put length(tData) into tLength
               delete char 1 to tLength of pData
               put item tLength of "4,7,10" into tBits
               put _format(baseConvert(tData, 10, 2), tBits) after sBitStream
            end repeat
            break
            
         case sModeAlpha
            local tVal1, tVal2, tBinary
            # add alphanumeric mode indicator and CCI to bit stream
            put "0010" & _format(baseConvert(tDataLength, 10, 2), tCCIBits) into sBitStream
            repeat until pData = empty
               put char 1 to 2 of pData into tData
               put length(tData) into tLength
               delete char 1 to tLength of pData
               if tLength = 2 then
                  put offset(char 1 of tData, sAlphaChars) - 1 into tVal1
                  put offset(char 2 of tData, sAlphaChars) - 1 into tVal2
                  put _format(baseConvert(tVal1 * 45+ tVal2, 10, 2), 11) into tBinary
               else
                  put offset(char 1 of tData, sAlphaChars) - 1 into tVal1
                  put _format(baseConvert(tVal1, 10, 2), 6) into tBinary
               end if
               put tBinary after sBitStream
            end repeat
            break
            
         case sModeByte
            local tVal
            # add 8 bit byte mode indicator and CCI to bit stream
            put "0100" & _format(baseConvert(tDataLength, 10, 2), tCCIBits) into sBitStream
            repeat for each char tChar in pData
               put charToNum(tChar) into tVal
               if tVal > 255 then put 32 into tVal
               put _format(baseConvert(tVal, 10, 2), 8) after sBitStream
            end repeat
            break
      end switch
      
      # add terminator if there's enough extra capacity
      local tBitLength, tXCap
      put length(sBitStream) into tBitLength
      put tBitStreamMax - tBitLength into tXCap
      if tXCap < 4 then 
         put _format("0", tXCap) after sBitStream
         add tXCap to tBitLength
      else
         put "0000" after sBitStream
         add 4 to tBitLength
      end if
      
      # pad to 8 bit multiple
      local tPad
      put tBitLength mod 8 into tPad
      if tPad > 0 then
         put 8 - tPad into tVal
         put _format("0", tVal) after sBitStream
         add tVal to tBitLength
      end if
      
      # add padding bytes 11101100 and 00010001 if there is extra capacity
      put "11101100" into tPad
      put (tBitStreamMax - tBitLength) div 8 into tXCap
      repeat tXCap
         put tPad after sBitStream
         add 8 to tBitLength
         if tPad = "11101100" then put "00010001" into tPad else put "11101100" into tPad
      end repeat
      
      # convert to byte stream and discard bit data
      put empty into sByteStream
      repeat until sBitStream = empty
         put numToChar(baseConvert(char 1 to 8 of sBitStream, 2, 10)) after sByteStream
         delete char 1 to 8 of sBitStream
      end repeat
      
      # calculate error correction codes per block
      local tCode, tECCData, tBlockIndex, tDataBlockA, tErrorBlockA, tBlocks, tDataWords
      put sVersion & "-" & sECC into tCode
      put line lineOffset(LF & tCode, LF & sECCDataList) of sECCDataList into tECCData
      put 0 into tBlockIndex
      put empty into tDataBlockA
      put empty into tErrorBlockA
      repeat for each item tBlockCountIndex in "3,6"
         put item tBlockCountIndex of tECCData into tBlocks
         if tBlocks > 0 then
            put item tBlockCountIndex + 1 of tECCData into tTotalWords
            put item tBlockCountIndex + 2 of tECCData into tDataWords
            put tTotalWords - tDataWords into tErrorWords
            repeat tBlocks
               add 1 to tBlockIndex
               put char 1 to tDataWords of sByteStream into tDataBlockA[tBlockIndex]
               delete char 1 to tDataWords of sByteStream
               put _errorCodes(tDataBlockA[tBlockIndex], tErrorWords) into tErrorBlockA[tBlockIndex]
            end repeat
         end if
      end repeat
      
      # construct the final codeword sequence
      put length(tDataBlockA[tBlockIndex]) into tDataLength -- last data blocks will always be of longest length
      local tErrorLength
      put length(tErrorBlockA[tBlockIndex]) into tErrorLength -- all error blocks should be of equal length
      put empty into sByteStream
      # add interleaved data blocks to stream
      repeat with j = 1 to tDataLength
         repeat with i = 1 to tBlockIndex
            put char j of tDataBlockA[i] into tChar
            if tChar <> empty then put tChar after sByteStream
         end repeat
      end repeat
      # add interleaved error blocks to stream
      repeat with j = 1 to tErrorLength
         repeat with i = 1 to tBlockIndex
            put char j of tErrorBlockA[i] after sByteStream
         end repeat
      end repeat
      
      # fill symbol capacity if required
      put item 2 of line sVersion of sCapacityList into tTotalWords
      local tStreamWords
      put length(sByteStream) into tStreamWords
      if tTotalWords > tStreamWords then
         put tTotalWords - tStreamWords into tPad
         repeat tPad
            put numToChar(0) after sByteStream
         end repeat
      end if
      
      # calc QR code size (no quiet zone) and image size (with quiet zone)
      put _size(sVersion) into sQRSize
      put sQRSize + 2 * sQZSize into sImgSize
      
      # initialize the target the image
      local tImgLoc
      if tPNG = false then
         put pImg into sQRCodeId
         put the loc of sQRCodeId into tImgLoc
         set the width of sQRCodeId to 1
         set the height of sQRCodeId to 1
         set the imageData of sQRCodeId to sWhitePixel
         set the width of sQRCodeId to sImgSize
         set the height of sQRCodeId to sImgSize
      end if
      
      # symbol map data - all used areas will be filled with '1'
      put empty into sSymbolMap
      repeat sQRSize * sQRSize
         put "0" after sSymbolMap
      end repeat
      # data map - will mirror the image data
      put sSymbolMap into sDataMap
      
      # generate image data
      put empty into sImgData
      repeat sImgSize ^ 2
         put sWhitePixel after sImgData
      end repeat
      
      # draw the position detection patterns
      # horizontal lines
      repeat with i = 1 to 7
         _draw i,1
         _draw i,2,1
         _draw i,7
         _draw i,6,1
         _draw sQRSize - i + 1, 1
         _draw sQRSize - i + 1, 2, 1
         _draw sQRSize - i + 1, 7
         _draw sQRSize - i + 1, 6, 1
         _draw i, sQRSize - 6
         _draw i, sQRSize - 5, 1
         _draw i, sQRSize
         _draw i, sQRSize - 1, 1
      end repeat
      #vertical lines
      repeat with i = 2 to 6
         _draw 1, i
         _draw 2, i, 1
         _draw 7, i
         _draw 6, i, 1
         _draw sQRSize - 6, i
         _draw sQRSize - 5, i, 1
         _draw sQRSize, i
         _draw sQRSize - 1, i, 1
         _draw 1, sQRSize - i + 1
         _draw 2, sQRSize - i + 1, 1
         _draw 7, sQRSize - i + 1
         _draw 6, sQRSize - i + 1, 1
      end repeat
      # middle boxes
      repeat with i = 3 to 5
         repeat with j = 3 to 5
            _draw i,j
            _draw sQRSize - i + 1, j
            _draw i, sQRSize - j + 1
         end repeat
      end repeat
      
      # draw timing patterns
      repeat with i = 9 to sQRSize - 8 step 2
         _draw i, 7
         _draw i + 1, 7, 1
         _draw 7, i
         _draw 7, i + 1, 1
      end repeat
      
      # spacing aroung position markers
      repeat with i = 1 to 8
         _draw i, 8, 1
         _draw 8, i, 1
         _draw i, sQRSize - 7, 1
         _draw 8, sQRSize - i + 1, 1
         _draw sQRSize - 7, i, 1
         _draw sQRSize - i + 1, 8, 1
      end repeat
      
      # draw alignment patterns
      local tPosData, tPosCount, tXVal, tYVal
      put item 3 to -1 of line sVersion of sAlignPosList into tPosData
      put the num of items in tPosData into tPosCount
      repeat with i = 1 to tPosCount
         put item i of tPosData + 1 into tXVal
         repeat with j = 1 to tPosCount
            put item j of tPosData + 1 into tYVal
            if (tXVal > 7 or tYVal > 7) and (tXVal < sQRSize - 7 or tYVal > 7) and (tYVal < sQRSize - 7 or tXVal > 7) then
               # valid alignment pattern position
               _draw tXVal, tYVal
               repeat with tX = tXVal - 2 to tXVal + 2
                  _draw tX, tYVal - 2
                  _draw tX, tYVal - 1, 1
                  _draw tX, tYVal + 2
                  _draw tX, tYVal + 1, 1
               end repeat
               repeat with tY = tYVal - 1 to tYVal + 1
                  _draw tXVal - 2, tY
                  _draw tXVal - 1, tY, 1
                  _draw tXVal + 2, tY
                  _draw tXVal + 1, tY, 1
               end repeat
            end if
         end repeat
      end repeat
      
      # format information
      repeat with i = 1 to 9
         _draw i, 9, 1, 1
         _draw 9, i, 1, 1
      end repeat
      repeat with i = 1 to 8
         _draw 9, sQRSize - i + 1, 1, 1
         _draw sQRSize - i + 1, 9, 1, 1
      end repeat
      
      # version information : only versions 7 - 40
      if sVersion > 6 then
         repeat with i = 1 to 3
            repeat with j = 1 to 6
               _draw sQRSize - 11 + i, j, 1, 1
               _draw j, sQRSize - 11 + i, 1, 1
            end repeat
         end repeat
      end if
      
      # keep a copy of the blank symbol map with no data for later use
      put sSymbolMap into sBlankMap
      
      # draw the data
      local tByteIndex, tXOffset, tDirection, tBit, tFreeModsFound, tColor
      put 1 into tByteIndex
      put 0 into tXOffset
      put -1 into tDirection
      put sQRSize into tX
      put sQRSize into tY
      put 7 into tBit
      put 0 into tFreeModsFound
      repeat until tX < 1
         
         # check the x,y position on the symbol map
         put char (tX - tXOffset) + (tY - 1) * sQRSize of sSymbolMap into tVal
         # check if the location is available
         if tVal = 0 then
            # available module on symbol for data
            add 1 to tFreeModsFound
            if (charToNum(char tByteIndex of sByteStream) bitAnd (2 ^ tBit)) > 0 then
               put 0 into tColor 
            else 
               put 1 into tColor
            end if
            _draw tX - tXOffset, tY, tColor
            subtract 1 from tBit
            if tBit < 0 then
               put 7 into tBit
               add 1 to tByteIndex 
            end if
         end if
         
         # move to next location
         add 1 to tXOffset
         if tXOffset > 1 then
            # next row
            put 0 into tXOffset
            add tDirection to tY
            if tDirection = -1 then
               if tY < 1 then
                  # next column
                  put 1 into tY
                  subtract 2 from tX
                  put 1 into tDirection
               end if
            else
               if tY > sQRSize then
                  # next column
                  put sQRSize into tY
                  subtract 2 from tX
                  put -1 into tDirection
               end if
            end if
            # make sure we skip the vertical timing marker
            if tX = 7 then 
               put 6 into tX
            end if
         end if
         
      end repeat
      
      # allow specific mask to be used!
      local tMask
      if pMask is an integer and pMask >= 0 and pMask <= 7 then
         put pMask into tMask
      else
         
         # generate mask patterns
         # WARNING!  Looking at the ISO doc it appears the i and j coordinates are reversed, so tH and tV have been switched!
         local tVRepeat, tPatA, tMap, tV, tPatMax, tH, tMaskVal
         put "2,2,1,3,4,6,6,6" into tVRepeat
         put empty into tPatA
         repeat with tMask = 0 to 7
            put empty into tMap
            repeat with i = 1 to sQRSize
               put i - 1 into tV
               # only generate the repeatable lines to fill the symbol
               put item tMask + 1 of tVRepeat into tPatMax
               repeat with j = 1 to tPatMax
                  put j - 1 into tH
                  put 0 into tMaskVal
                  switch tMask
                     case 0
                        if (tH + tV) mod 2 = 0 then put 1 into tMaskVal
                        break
                     case 1
                        if tH mod 2 = 0 then put 1 into tMaskVal
                        break
                     case 2
                        if tV mod 3 = 0 then put 1 into tMaskVal
                        break
                     case 3
                        if (tH + tV) mod 3 = 0 then put 1 into tMaskVal
                        break
                     case 4
                        if ((tH div 2) + (tV div 3)) mod 2 = 0 then put 1 into tMaskVal
                        break
                     case 5
                        if ((tH * tV) mod 2) + ((tH * tV) mod 3) = 0 then put 1 into tMaskVal
                        break
                     case 6
                        if (((tH * tV) mod 2) + ((tH * tV) mod 3)) mod 2 = 0 then put 1 into tMaskVal
                        break
                     case 7
                        if (((tH * tV) mod 3) + ((tH + tV) mod 2)) mod 2 = 0 then put 1 into tMaskVal
                        break
                  end switch
                  put tMaskVal into char i of line j of tMap
               end repeat
            end repeat
            replace LF with empty in tMap
            put tMap into tPatA[tMask]
         end repeat
         
         # apply each mask to the data map
         local tPatLine, tPos, tExVal, tMapA
         repeat with tMask = 0 to 7
            put sDataMap into tMap
            repeat with i = 1 to sQRSize
               # number of lines in the mask pattern before it repeats
               put item tMask + 1 of tVRepeat into tPatMax
               # current pattern line
               put 1 into tPatLine
               repeat with j = 1 to sQRSize
                  put i + (j - 1) * sQRSize into tPos
                  if char tPos of sBlankMap = 0 then
                     put char tPos of tMap into tExVal
                     put char i + (tPatLine - 1) * sQRSize of tPatA[tMask] into tMaskVal
                     if tMaskVal = 1 then
                        put tExVal bitXOr 1 into char tPos of tMap
                     end if
                  end if
                  add 1 to tPatLine
                  if tPatLine > tPatMax then put 1 into tPatLine
               end repeat
            end repeat
            put tMap into tMapA[tMask]
         end repeat
         
         # score the masks
         local tPenaltyA, tMaxPos, tPrevChar, tMatches
         repeat with tMask = 0 to 7
            put tMapA[tMask] into tMap
            replace LF with empty in tMap
            put 0 into tPenaltyA[tMask]
            
            # score 1: adjacent modules in row/column in same color
            put sQRSize * sQRSize into tMaxPos
            put empty into tPrevChar
            put 0 into tMatches
            repeat with i = 1 to tMaxPos
               put char i of tMap into tChar
               if i mod sQRSize = 1 then
                  if tMatches > 4 then
                     add 3 + (tMatches - 5) to tPenaltyA[tMask]
                  end if
                  put tChar into tPrevChar
                  put 1 into tMatches
               else
                  if tChar = tPrevChar then
                     add 1 to tMatches
                  else
                     if tMatches > 4 then
                        add 3 + (tMatches - 5) to tPenaltyA[tMask]
                     end if
                     put tChar into tPrevChar
                     put 1 into tMatches
                  end if
               end if
            end repeat
            if tMatches > 4 then
               add 3 + (tMatches - 5) to tPenaltyA[tMask]
            end if
            
            # score 2: block of modules the same color
            put sQRSize * sQRSize - sQRSize - 1 into tMaxPos
            local tChars
            repeat with i = 1 to tMaxPos
               # don't search when on the right edge
               if i mod sQRSize <> 0 then
                  put char i to i + 1 of tMap into tChars
                  put char i + sQRSize to i + sQRSize + 1 of tMap after tChars
                  if tChars = "0000" or tChars = "1111" then add 3 to tPenaltyA[tMask]
               end if
            end repeat
            
            # score 3: 1-1-3-1-1 pattern in row/column
            # create new rows from the column data and append to the map data
            local tLine
            repeat with i = 1 to sQRSize
               put empty into tLine
               repeat with j = 1 to sQRSize
                  put char i + (j - 1) * sQRSize of tMap after tLine
               end repeat
               put tLine after tMap
            end repeat
            put sQRSize * sQRSize * 2 into tMaxPos
            repeat with i = 1 to tMaxPos step sQRSize
               put char i to i + sQRSize - 1 of tMap into tLine
               # check for position pattern with quiet zone on each side
               local tOffset
               put -1 into tOffset
               repeat until tOffset = 0
                  put offset("000010111010000", tLine) into tOffset
                  if tOffset > 0 then
                     delete char tOffset to tOffset + 14 of tLine
                     add 40 to tPenaltyA[tMask]
                  end if
               end repeat
               # check for position pattern with quiet zone on right
               put -1 into tOffset
               repeat until tOffset = 0
                  put offset("10111010000", tLine) into tOffset
                  if tOffset > 0 then
                     delete char tOffset to tOffset + 10 of tLine
                     add 40 to tPenaltyA[tMask]
                  end if
               end repeat
               # check for position pattern with quiet zone on left
               put -1 into tOffset
               repeat until tOffset = 0
                  put offset("00001011101", tLine) into tOffset
                  if tOffset > 0 then
                     delete char tOffset to tOffset + 10 of tLine
                     add 40 to tPenaltyA[tMask]
                  end if
               end repeat
            end repeat
            # remove extra rows
            put sQRSize * sQRSize into tMaxPos
            delete char tMaxPos + 1 to -1 of tMap
            
            # score 4: ratio of dark to light pixels
            local tTotal, tDark
            put the num of chars in tMap into tTotal
            replace 0 with empty in tMap
            put the num of chars in tMap into tDark
            add abs(trunc(100 * (tDark / tTotal) - 50))  / 5 * 10 to tPenaltyA[tMask]
            
         end repeat
         
         # choose the mask with the lowest penalty
         put 0 into tMask
         repeat with i = 1 to 7
            if tPenaltyA[i] < tPenaltyA[tMask] then put i into tMask
         end repeat
         
      end if -- specify mask pattern
      
      
      # apply chosen mask to image - QUICKER
      # WARNING!  Looking at the ISO doc it appears the i and j coordinates are reversed, so tH and tV have been switched!
      repeat with i = 1 to sQRSize
         put i - 1 into tV
         repeat with j = 1 to sQRSize
            put j - 1 into tH
            put i + (j - 1) * sQRSize into tPos
            if char tPos of sBlankMap = 0 then
               put char tPos of sDataMap into tExVal
               put 0 into tMaskVal
               switch tMask
                  case 0
                     if (tH + tV) mod 2 = 0 then put 1 into tMaskVal
                     break
                  case 1
                     if tH mod 2 = 0 then put 1 into tMaskVal
                     break
                  case 2
                     if tV mod 3 = 0 then put 1 into tMaskVal
                     break
                  case 3
                     if (tH + tV) mod 3 = 0 then put 1 into tMaskVal
                     break
                  case 4
                     if ((tH div 2) + (tV div 3)) mod 2 = 0 then put 1 into tMaskVal
                     break
                  case 5
                     if ((tH * tV) mod 2) + ((tH * tV) mod 3) = 0 then put 1 into tMaskVal
                     break
                  case 6
                     if (((tH * tV) mod 2) + ((tH * tV) mod 3)) mod 2 = 0 then put 1 into tMaskVal
                     break
                  case 7
                     if (((tH * tV) mod 3) + ((tH + tV) mod 2)) mod 2 = 0 then put 1 into tMaskVal
                     break
               end switch
               if tMaskVal = 1 then
                  if tExVal = 1 then put 1 into tColor else put 0 into tColor
                  _draw i, j, tColor
               end if
            end if
         end repeat
      end repeat
      
      # add format data
      local tTmpString
      local tFormatBits, tFormatPos
      put item 2 of line lineOffset(LF & sECC & tMask, LF & sFormatList) of sFormatList into tFormatBits
      put "1,9" & LF after tTmpString
      put "2,9" & LF after tTmpString
      put "3,9" & LF after tTmpString
      put "4,9" & LF after tTmpString
      put "5,9" & LF after tTmpString
      put "6,9" & LF after tTmpString
      put "8,9" & LF after tTmpString
      put "9,9" & LF after tTmpString
      put "9,8" & LF after tTmpString
      put "9,6" & LF after tTmpString
      put "9,5" & LF after tTmpString
      put "9,4" & LF after tTmpString
      put "9,3" & LF after tTmpString
      put "9,2" & LF after tTmpString
      put "9,1" after tTmpString
      put tTmpString into tFormatPos
      put empty into tTmpString
      
      repeat with i = 1 to 15
         put item 1 of line i of tFormatPos into tX
         put item 2 of line i of tFormatPos into tY
         put char i of tFormatBits into tBit
         if tBit = 0 then put 1 into tColor else put 0 into tColor
         _draw tX, tY, tColor
      end repeat
      repeat with i = 1 to 7
         put char i of tFormatBits into tBit
         if tBit = 0 then put 1 into tColor else put 0 into tColor
         _draw 9, sQRSize - i + 1, tColor
      end repeat
      repeat with i = 8 to 15
         put char i of tFormatBits into tBit
         if tBit = 0 then put 1 into tColor else put 0 into tColor
         _draw sQRSize - 15 + i, 9, tColor
      end repeat
      # dark module
      _draw 9, sQRSize - 7, 0
      
      # add version data
      local tVLine, tVersionBits, tVIndex
      if sVersion > 6 then
         put lineOffset(LF & sVersion, LF & sVersionList) into tVLine
         put item 2 of line tVLine of sVersionList into tVersionBits
         put 1 into tVIndex
         repeat with i = 1 to 6
            repeat with j = 1 to 3
               if char tVIndex of tVersionBits = 1 then put 0 into tColor else put 1 into tColor
               _draw i, sQRSize - 11 + j, tColor
               _draw sQRSize - 11 + j, i, tColor
               add 1 to tVIndex
            end repeat
         end repeat
      end if
      
      # image file or object update
      local tPNGData
      if tPNG = true then
         pngCreate pImg, pSize
         put LF & the result into tPNGData
      else
         # update the image data
         set the imageData of sQRCodeId to sImgData
         
         # resize
         if pSize is an integer and pSize > 1 then
            local tSize
            put sImgSize * pSize into tSize
            set the width of sQRCodeId to tSize
            set the height of sQRCodeId to tSize
         end if
         
         # reposition to original location
         set the loc of sQRCodeId to tImgLoc
      end if
      
   catch tError
      return "Error: " & tError
   finally
      unlock screen
   end try
   
   return sVersion & comma & sECC & comma & sModeNamesA[sMode] & comma & tMask & comma & sImgSize & tPNGData
end qrCreate


private function _checkColor pColor
   local tItems
   put the num of items in pColor into tItems
   if tItems = 1 then
      local tOffset
      put lineOffset(LF & pColor, LF & sColorNames) into tOffset
      if tOffset > 0 then
         return item 2 to 4 of line tOffset of sColorNames
      end if
   else if tItems = 3 then
      local tItem
      repeat with i = 1 to 3
         put item i of pColor into tItem
         if tItem is not an integer or tItem < 0 or tItem > 255 then
            return false
         end if
      end repeat
      return pColor
   end if
   return false
end _checkColor

/**

Set colors other than the default black and white

Example:
qrSetColors "Blue", "Yellow"

Parameters:
pForeground (string):
A color name or RGB triplet

pBackground (string):
A color name or RGB triplet

Description:
Optionally use colors other than black and white there must be a high
enough contrast between colors for scanning the foreground color
should be darker than the background color.

*/

command qrSetColors pForeground, pBackground
   local tColor
   put _checkColor(pForeground) into tColor
   if tColor <> false then
      put _numsToBytes("0" & comma & tColor) into sBlackPixel
      put _numsToBytes(tColor) into sPNGBlack
   end if
   put _checkColor(pBackground) into tColor
   if tColor <> false then
      put _numsToBytes("0" & comma & tColor) into sWhitePixel
      put _numsToBytes(tColor) into sPNGWhite
   end if
end qrSetColors


private command pngCreate pFile, pSize
   # check size
   if not(pSize is an integer and pSize > 1) then put 1 into pSize
   
   # png header
   local tByteStream
   put _numsToBytes("137,80,78,71,13,10,26,10") into tByteStream
   
   # IHDR (13 bytes)
   put _hexToBytes(format("%08x", 13)) after tByteStream -- length
   local tData
   put "IHDR" into tData
   local tWidth
   put sImgSize * pSize into tWidth
   local tHeight
   put tWidth into tHeight
   put _hexToBytes(format("%08x", tWidth)) after tData
   put _hexToBytes(format("%08x", tHeight)) after tData
   put numToChar(1) after tData -- bit depth
   put numToChar(3) after tData -- color type: indexed
   put numToChar(0) after tData -- compression method: inflate/deflate
   put numToChar(0) after tData -- filter method: adaptive
   put numToChar(0) after tData -- interlace method: none
   put tData & _crc32(tData) after tByteStream
   
   # PLTE : 1 bit palette
   put _hexToBytes(format("%08x", 6)) after tByteStream -- length
   put "PLTE" into tData
   put sPNGBlack & sPNGWhite after tData
   put tData & _crc32(tData) after tByteStream
   
   # IDAT : image data
   local tQZ
   put _format("0", sQRSize * 4) into tQZ
   put tQZ before sDataMap
   put tQZ after sDataMap
   local tLine, tLen, tChar, tChars
   repeat until sDataMap = empty
      # filter type 0 : none
      put "0000" & char 1 to sQRSize of sDataMap & "0000" into tLine
      replace "0" with "W" in tLine
      replace "1" with "0" in tLine
      replace "W" with "1" in tLine
      delete char 1 to sQRSize of sDataMap
      # expand size
      put length(tLine) into tLen
      repeat with i = tLen to 1 step -1
         put char i of tLine into tChar
         put empty into tChars
         repeat pSize
            put tChar after tChars
         end repeat
         put tChars into char i of tLine
      end repeat
      # pad to 8 bit multiple
      local tPad, tVal, tBinLine, tBinary, tIDAT
      put length(tLine) mod 8 into tPad
      if tPad > 0 then
         put 8 - tPad into tVal
         put _format("0", tVal) after tLine
      end if
      # convert to bytes
      put empty into tBinLine
      repeat until tLine = empty
         put char 1 to 8 of tLine into tBinary
         delete char 1 to 8 of tLine
         put numToChar(baseConvert(tBinary, 2, 10)) after tBinLine
      end repeat
      repeat pSize
         put numToChar(0) & tBinLine after tIDAT
      end repeat
   end repeat
   put _deflate(tIDAT) into tIDAT
   put _hexToBytes(format("%08x", length(tIDAT))) after tByteStream -- length
   put "IDAT" into tData
   put tIDAT after tData
   put tData & _crc32(tData) after tByteStream
   
   # IEND
   put _hexToBytes(format("%08x", 0)) after tByteStream -- length
   put "IEND" into tData
   put tData & _crc32(tData) after tByteStream
   
   # write png file
   if pFile = "!" then
      return tByteStream
   else
      put tByteStream into URL("binfile:" &  pFile)
   end if
end pngCreate


# add a comma separated list of numbers to png data
private function _numsToBytes pData
   local tResult
   repeat for each item tCode in pData
      put numToChar(tCode) after tResult
   end repeat
   return tResult
end _numsToBytes


# add hex data to png data
private function _hexToBytes pData
   local tHex, tResult
   repeat until pData = empty
      put char 1 to 2 of pData into tHex
      put numToChar("0x" & tHex) after tResult
      delete char 1 to 2 of pData
   end repeat
   return tResult
end _hexToBytes


private command _crcMakeTable
   put empty into sCrcTableA
   repeat with tIndex = 0 to 255
      local tVal
      put tIndex into tVal
      repeat with tBit = 0 to 7
         if (tVal bitAnd 1) > 0 then
            put 3988292384 bitXor (tVal div 2) into tVal
         else
            put tVal div 2 into tVal
         end if
      end repeat
      put tVal into sCrcTableA[tIndex]
   end repeat
   put true into sCrcTableCreated
end _crcMakeTable


private function _crcUpdate pCrc, @pData
   if sCrcTableCreated <> true then _crcMakeTable
   local tLen
   put length(pData) into tLen
   repeat with tIndex = 1 to tLen
      put sCrcTableA[(pCrc bitXor charToNum(char tIndex of pData)) bitAnd 255] bitXor (pCrc div 256) into pCrc
   end repeat
   return pCrc
end _crcUpdate


private function _crc32 pData
   local tResult, tBytes
   put _crcUpdate(4294967295, pData) bitXor 4294967295 into tResult
   repeat until tResult = 0
      put numToChar(tResult bitAnd 255) before tBytes
      put tResult div 256 into tResult
   end repeat
   return tBytes
end _crc32


private function _deflate pData
   local tComp, tVal, tA, tB, tA1, tA2, tB1, tB2
   put compress(pData) into tComp
   delete char 1 to 10 of tComp
   delete char -8 to -1 of tComp
   put 1 into tA
   put 0 into tB
   repeat for each char tChar in pData
      put charToNum(tChar) into tVal
      add tVal to tA
      if tA > 65521 then subtract 65521 from tA
      add tA to tB
      if tB > 65521 then subtract 65521 from tB
   end repeat
   put numToChar((tA bitAnd 65280) / 256) into tA1
   put numToChar(tA bitAnd 255) into tA2
   put numToChar((tB bitAnd 65280) / 256) into tB1
   put numToChar(tB bitAnd 255) into tB2
   put numToChar(120) & numToChar(156) before tComp
   put tB1 & tB2 & tA1 & tA2 after tComp
   return tComp
end _deflate
