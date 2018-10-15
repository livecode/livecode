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

typedef struct
{
	const char *token;
	uint1 red;
	uint1 green;
	uint1 blue;
}
MCRGB;

#define NAME_LENGTH 6000

static const MCRGB color_table[] =
    {
        {"AliceBlue", 239, 247, 255},
        {"AntiqueWhite", 249, 232, 210},
        {"AntiqueWhite1", 254, 237, 214},
        {"AntiqueWhite2", 235, 219, 197},
        {"AntiqueWhite3", 200, 185, 166},
        {"AntiqueWhite4", 129, 116, 104},
        {"Aquamarine", 67, 183, 186},
        {"Aquamarine1", 127, 255, 212},
        {"Aquamarine2", 118, 238, 198},
        {"Aquamarine3", 102, 205, 170},
        {"Aquamarine4", 65, 124, 100},
        {"Azure", 239, 255, 255},
        {"Azure1", 239, 255, 255},
        {"Azure2", 222, 236, 236},
        {"Azure3", 188, 199, 199},
        {"Azure4", 122, 125, 125},
        {"Beige", 245, 243, 215},
        {"Bisque", 255, 228, 196},
        {"Bisque1", 253, 224, 188},
        {"Bisque2", 234, 208, 174},
        {"Bisque3", 199, 175, 146},
        {"Bisque4", 129, 110, 89},
        {"Black", 0, 0, 0},
        {"BlanchedAlmond", 254, 232, 198},
        {"Blue", 0, 0, 255},
        {"Blue1", 21, 53, 255},
        {"Blue2", 21, 49, 236},
        {"Blue3", 21, 40, 199},
        {"Blue4", 21, 27, 126},
        {"BlueViolet", 138, 43, 226},
        {"Brown", 152, 5, 23},
        {"Brown1", 246, 53, 38},
        {"Brown2", 228, 45, 23},
        {"Brown3", 194, 34, 23},
        {"Brown4", 126, 5, 23},
        {"Burlywood", 216, 175, 121},
        {"Burlywood1", 252, 206, 142},
        {"Burlywood2", 234, 190, 131},
        {"Burlywood3", 198, 160, 109},
        {"Burlywood4", 128, 99, 65},
        {"CadetBlue", 87, 134, 147},
        {"CadetBlue1", 152, 245, 255},
        {"CadetBlue2", 142, 226, 236},
        {"CadetBlue3", 119, 191, 199},
        {"CadetBlue4", 76, 120, 126},
        {"Chartreuse", 127, 255, 0},
        {"Chartreuse1", 127, 255, 0},
        {"Chartreuse2", 118, 238, 0},
        {"Chartreuse3", 102, 205, 0},
        {"Chartreuse4", 67, 124, 23},
        {"Chocolate", 200, 90, 23},
        {"Chocolate1", 248, 114, 23},
        {"Chocolate2", 229, 103, 23},
        {"Chocolate3", 195, 86, 23},
        {"Chocolate4", 126, 49, 23},
        {"Coral", 247, 101, 65},
        {"Coral1", 247, 101, 65},
        {"Coral2", 229, 91, 60},
        {"Coral3", 195, 74, 44},
        {"Coral4", 126, 40, 23},
        {"CornflowerBlue", 100, 149, 237},
        {"CornSilk", 255, 247, 215},
        {"CornSilk1", 255, 247, 215},
        {"CornSilk2", 236, 229, 198},
        {"CornSilk3", 200, 194, 167},
        {"CornSilk4", 129, 122, 104},
        {"Cyan", 0, 255, 255},
        {"Cyan1", 87, 254, 255},
        {"Cyan2", 80, 235, 236},
        {"Cyan3", 70, 199, 199},
        {"Cyan4", 48, 125, 126},
        {"DarkBlue", 0, 0, 139},
        {"DarkCyan", 0, 139, 139},
        {"DarkGoldenrod", 175, 120, 23},
        {"DarkGoldenrod1", 251, 177, 23},
        {"DarkGoldenrod2", 232, 163, 23},
        {"DarkGoldenrod3", 197, 137, 23},
        {"DarkGoldenrod4", 127, 82, 23},
        {"DarkGray", 169, 169, 169},
        {"DarkGreen", 0, 100, 0},
        {"DarkKhaki", 183, 173, 89},
        {"DarkMagenta", 139, 0, 139},
        {"DarkOliveGreen", 74, 65, 23},
        {"DarkOliveGreen1", 202, 255, 112},
        {"DarkOliveGreen2", 188, 233, 84},
        {"DarkOliveGreen3", 160, 197, 68},
        {"DarkOliveGreen4", 102, 124, 38},
        {"DarkOrange", 248, 128, 23},
        {"DarkOrange1", 248, 114, 23},
        {"DarkOrange2", 229, 103, 23},
        {"DarkOrange3", 195, 86, 23},
        {"DarkOrange4", 126, 49, 23},
        {"DarkOrchid", 125, 27, 126},
        {"DarkOrchid1", 176, 65, 255},
        {"DarkOrchid2", 162, 59, 236},
        {"DarkOrchid3", 139, 49, 199},
        {"DarkOrchid4", 104, 34, 139},
        {"DarkRed", 139, 0, 0},
        {"DarkSalmon", 225, 139, 107},
        {"DarkSeaGreen", 139, 179, 129},
        {"DarkSeaGreen1", 193, 255, 193},
        {"DarkSeaGreen2", 180, 238, 180},
        {"DarkSeaGreen3", 153, 198, 142},
        {"DarkSeaGreen4", 105, 139, 105},
        {"DarkSlateBlue", 43, 56, 86},
        {"DarkSlateGray", 37, 56, 60},
        {"DarkSlateGray1", 151, 255, 255},
        {"DarkSlateGray2", 141, 238, 238},
        {"DarkSlateGray3", 120, 199, 199},
        {"DarkSlateGray4", 76, 125, 126},
        {"DarkTurquoise", 59, 156, 156},
        {"DarkViolet", 132, 45, 206},
        {"DeepPink", 245, 40, 135},
        {"DeepPink1", 255, 20, 147},
        {"DeepPink2", 228, 40, 124},
        {"DeepPink3", 193, 34, 103},
        {"DeepPink4", 125, 5, 63},
        {"DeepSkyBlue", 0, 191, 255},
        {"DeepSkyBlue1", 59, 185, 255},
        {"DeepSkyBlue2", 0, 178, 238},
        {"DeepSkyBlue3", 0, 154, 205},
        {"DeepSkyBlue4", 0, 104, 139},
        {"DimGray", 70, 62, 65},
        {"DodgerBlue", 21, 137, 255},
        {"DodgerBlue1", 30, 144, 255},
        {"DodgerBlue2", 28, 134, 238},
        {"DodgerBlue3", 24, 116, 205},
        {"DodgerBlue4", 16, 78, 139},
        {"Firebrick", 128, 5, 23},
        {"Firebrick1", 246, 40, 23},
        {"Firebrick2", 228, 34, 23},
        {"Firebrick3", 193, 27, 23},
        {"Firebrick4", 126, 5, 23},
        {"FloralWhite", 255, 249, 238},
        {"ForestGreen", 34, 139, 34},
        {"Gainsboro", 216, 217, 215},
        {"GhostWhite", 247, 247, 255},
        {"Gold", 212, 160, 23},
        {"Gold1", 253, 208, 23},
        {"Gold2", 234, 193, 23},
        {"Gold3", 199, 163, 23},
        {"Gold4", 128, 101, 23},
        {"Goldenrod", 218, 165, 32},
        {"Goldenrod1", 251, 185, 23},
        {"Goldenrod2", 233, 171, 23},
        {"Goldenrod3", 198, 142, 23},
        {"Goldenrod4", 128, 88, 23},
		{"Gray", 190, 190, 190},
		{"Gray0", 0, 0, 0}, 
		{"Gray1", 3, 3, 3},
		{"Gray10", 26, 26, 26},
		{"Gray100", 255, 255, 255},
		{"Gray11", 28, 28, 28},
		{"Gray12", 31, 31, 31},
		{"Gray13", 33, 33, 33},
		{"Gray14", 36, 36, 36},
		{"Gray15", 38, 38, 38},
		{"Gray16", 41, 41, 41},
		{"Gray17", 43, 43, 43},
		{"Gray18", 46, 46, 46},
		{"Gray19", 48, 48, 48},
		{"Gray2", 5, 5, 5},
		{"Gray20", 51, 51, 51},
		{"Gray21", 54, 54, 54},
		{"Gray22", 56, 56, 56},
		{"Gray23", 59, 59, 59},
		{"Gray24", 61, 61, 61},
		{"Gray25", 64, 64, 64},
		{"Gray26", 66, 66, 66},
		{"Gray27", 69, 69, 69},
		{"Gray28", 71, 71, 71},
		{"Gray29", 74, 74, 74},
		{"Gray3", 8, 8, 8},
		{"Gray30", 77, 77, 77},
		{"Gray31", 79, 79, 79}, 
		{"Gray32", 82, 82, 82}, 
		{"Gray33", 84, 84, 84}, 
		{"Gray34", 87, 87, 87}, 
		{"Gray35", 89, 89, 89}, 
		{"Gray36", 92, 92, 92}, 
		{"Gray37", 94, 94, 94}, 
		{"Gray38", 97, 97, 97}, 
		{"Gray39", 99, 99, 99}, 
		{"Gray4", 10, 10, 10},
		{"Gray40", 102, 102, 102},
		{"Gray41", 105, 105, 105}, 
		{"Gray42", 107, 107, 107}, 
		{"Gray43", 110, 110, 110}, 
		{"Gray44", 112, 112, 112}, 
		{"Gray45", 115, 115, 115}, 
		{"Gray46", 117, 117, 117}, 
		{"Gray47", 120, 120, 120}, 
		{"Gray48", 122, 122, 122}, 
		{"Gray49", 125, 125, 125}, 
		{"Gray5", 13, 13, 13},
		{"Gray50", 128, 128, 128},
		{"Gray51", 130, 130, 130}, 
		{"Gray52", 133, 133, 133}, 
		{"Gray53", 135, 135, 135}, 
		{"Gray54", 138, 138, 138}, 
		{"Gray55", 140, 140, 140}, 
		{"Gray56", 143, 143, 143}, 
		{"Gray57", 145, 145, 145}, 
		{"Gray58", 148, 148, 148}, 
		{"Gray59", 150, 150, 150}, 
		{"Gray6", 15, 15, 15},
		{"Gray60", 153, 153, 153},
		{"Gray61", 156, 156, 156}, 
		{"Gray62", 158, 158, 158}, 
		{"Gray63", 161, 161, 161}, 
		{"Gray64", 163, 163, 163}, 
		{"Gray65", 166, 166, 166}, 
		{"Gray66", 168, 168, 168}, 
		{"Gray67", 171, 171, 171}, 
		{"Gray68", 173, 173, 173}, 
		{"Gray69", 176, 176, 176}, 
		{"Gray7", 18, 18, 18},
		{"Gray70", 179, 179, 179},
		{"Gray71", 181, 181, 181}, 
		{"Gray72", 184, 184, 184}, 
		{"Gray73", 186, 186, 186}, 
		{"Gray74", 189, 189, 189}, 
		{"Gray75", 191, 191, 191}, 
		{"Gray76", 194, 194, 194}, 
		{"Gray77", 196, 196, 196}, 
		{"Gray78", 199, 199, 199}, 
		{"Gray79", 201, 201, 201}, 
		{"Gray8", 20, 20, 20},
		{"Gray80", 204, 204, 204},
		{"Gray81", 207, 207, 207}, 
		{"Gray82", 209, 209, 209}, 
		{"Gray83", 212, 212, 212}, 
		{"Gray84", 214, 214, 214}, 
		{"Gray85", 217, 217, 217}, 
		{"Gray86", 219, 219, 219}, 
		{"Gray87", 222, 222, 222}, 
		{"Gray88", 224, 224, 224}, 
		{"Gray89", 227, 227, 227}, 
		{"Gray9", 23, 23, 23},
		{"Gray90", 230, 230, 230},
		{"Gray91", 232, 232, 232}, 
		{"Gray92", 235, 235, 235}, 
		{"Gray93", 237, 237, 237}, 
		{"Gray94", 240, 240, 240}, 
		{"Gray95", 242, 242, 242}, 
		{"Gray96", 245, 245, 245}, 
		{"Gray97", 247, 247, 247}, 
		{"Gray98", 250, 250, 250}, 
		{"Gray99", 252, 252, 252},
        {"Green", 0, 255, 0},
        {"Green1", 95, 251, 23},
        {"Green2", 0, 238, 0},
        {"Green3", 0, 205, 0},
        {"Green4", 0, 139, 0},
        {"GreenYellow", 173, 255, 47},
        {"Honeydew", 240, 254, 238},
        {"Honeydew1", 240, 255, 240},
        {"Honeydew2", 222, 235, 220},
        {"Honeydew3", 188, 199, 185},
        {"Honeydew4", 122, 125, 116},
        {"HotPink", 246, 96, 171},
        {"HotPink1", 246, 101, 171},
        {"HotPink2", 228, 94, 157},
        {"HotPink3", 194, 82, 131},
        {"HotPink4", 125, 34, 82},
        {"IndianRed", 205, 92, 92},
        {"IndianRed1", 247, 93, 89},
        {"IndianRed2", 229, 84, 81},
        {"IndianRed3", 194, 70, 65},
        {"IndianRed4", 126, 34, 23},
        {"Ivory", 255, 255, 238},
        {"Ivory1", 255, 255, 238},
        {"Ivory2", 236, 236, 220},
        {"Ivory3", 201, 199, 185},
        {"Ivory4", 129, 125, 116},
        {"Khaki", 173, 169, 110},
        {"Khaki1", 255, 243, 128},
        {"Khaki2", 237, 226, 117},
        {"Khaki3", 201, 190, 98},
        {"Khaki4", 130, 120, 57},
        {"Lavender", 227, 228, 250},
        {"LavenderBlush", 253, 238, 244},
        {"LavenderBlush1", 255, 240, 245},
        {"LavenderBlush2", 235, 221, 226},
        {"LavenderBlush3", 200, 187, 190},
        {"LavenderBlush4", 129, 118, 121},
        {"LawnGreen", 124, 252, 0},
        {"LemonChiffon", 255, 248, 198},
        {"LemonChiffon1", 255, 250, 205},
        {"LemonChiffon2", 238, 233, 191},
        {"LemonChiffon3", 205, 201, 165},
        {"LemonChiffon4", 139, 137, 112},
        {"LightBlue", 173, 216, 230},
        {"LightBlue1", 189, 237, 255},
        {"LightBlue2", 175, 220, 236},
        {"LightBlue3", 149, 185, 199},
        {"LightBlue4", 104, 131, 139},
        {"LightCoral", 231, 116, 113},
        {"LightCyan", 224, 255, 255},
        {"LightCyan1", 224, 255, 255},
        {"LightCyan2", 207, 236, 236},
        {"LightCyan3", 175, 199, 199},
        {"LightCyan4", 113, 125, 125},
        {"LightGoldenrod", 236, 216, 114},
        {"LightGoldenrod1", 255, 232, 124},
        {"LightGoldenrod2", 236, 214, 114},
        {"LightGoldenrod3", 200, 181, 96},
        {"LightGoldenrod4", 129, 115, 57},
        {"LightGoldenrodYellow", 250, 248, 204},
        {"LightGray", 160, 159, 157},
        {"LightGreen", 144, 238, 144},
        {"LightPink", 250, 175, 186},
        {"LightPink1", 249, 167, 176},
        {"LightPink2", 231, 153, 163},
        {"LightPink3", 196, 129, 137},
        {"LightPink4", 127, 78, 82},
        {"LightSalmon", 249, 150, 107},
        {"LightSalmon1", 255, 160, 122},
        {"LightSalmon2", 231, 138, 97},
        {"LightSalmon3", 196, 116, 81},
        {"LightSalmon4", 127, 70, 44},
        {"LightSeaGreen", 32, 178, 170},
        {"LightSkyBlue", 130, 202, 250},
        {"LightSkyBlue1", 173, 223, 255},
        {"LightSkyBlue2", 160, 207, 236},
        {"LightSkyBlue3", 135, 175, 199},
        {"LightSkyBlue4", 86, 109, 126},
        {"LightSlateBlue", 115, 106, 255},
        {"LightSlateGray", 109, 123, 141},
        {"LightSteelBlue", 114, 143, 206},
        {"LightSteelBlue1", 198, 222, 255},
        {"LightSteelBlue2", 183, 206, 236},
        {"LightSteelBlue3", 154, 173, 199},
        {"LightSteelBlue4", 100, 109, 126},
        {"LightYellow", 255, 254, 220},
        {"LightYellow1", 255, 255, 224},
        {"LightYellow2", 237, 235, 203},
        {"LightYellow3", 201, 199, 170},
        {"LightYellow4", 130, 125, 107},
        {"LimeGreen", 50, 205, 50},
        {"Linen", 249, 238, 226},
        {"Magenta", 244, 62, 255},
        {"Magenta1", 255, 0, 255},
        {"Magenta2", 226, 56, 236},
        {"Magenta3", 192, 49, 199},
        {"Magenta4", 125, 27, 126},
        {"Maroon", 129, 5, 65},
        {"Maroon1", 245, 53, 170},
        {"Maroon2", 227, 49, 157},
        {"Maroon3", 193, 34, 131},
        {"Maroon4", 125, 5, 82},
        {"MediumAquamarine", 102, 205, 170},
        {"MediumBlue", 0, 0, 205},
        {"MediumForestGreen", 52, 114, 53},
        {"MediumGoldenrod", 204, 185, 84},
        {"MediumOrchid", 176, 72, 181},
        {"MediumOrchid1", 212, 98, 255},
        {"MediumOrchid2", 196, 90, 236},
        {"MediumOrchid3", 167, 74, 199},
        {"MediumOrchid4", 106, 40, 126},
        {"MediumPurple", 132, 103, 215},
        {"MediumPurple1", 171, 130, 255},
        {"MediumPurple2", 159, 121, 238},
        {"MediumPurple3", 137, 104, 205},
        {"MediumPurple4", 93, 71, 139},
        {"MediumSeaGreen", 48, 103, 84},
        {"MediumSlateBlue", 123, 104, 238},
        {"MediumSpringGreen", 0, 250, 154},
        {"MediumTurquoise", 72, 204, 205},
        {"MediumVioletRed", 199, 21, 133},
        {"MidnightBlue", 21, 27, 84},
        {"MintCream", 245, 255, 249},
        {"MistyRose", 253, 225, 221},
        {"MistyRose1", 255, 228, 225},
        {"MistyRose2", 234, 208, 204},
        {"MistyRose3", 198, 175, 172},
        {"MistyRose4", 128, 111, 108},
        {"Moccasin", 253, 224, 172},
        {"NavajoWhite", 253, 218, 163},
        {"NavajoWhite1", 255, 222, 173},
        {"NavajoWhite2", 234, 201, 149},
        {"NavajoWhite3", 199, 170, 125},
        {"NavajoWhite4", 128, 106, 75},
        {"Navy", 0, 0, 128},
        {"NavyBlue", 0, 0, 128},
        {"OldLace", 252, 243, 226},
        {"OliveDrab", 101, 128, 23},
        {"OliveDrab1", 192, 255, 62},
        {"OliveDrab2", 179, 238, 58},
        {"OliveDrab3", 153, 197, 23},
        {"OliveDrab4", 105, 139, 34},
        {"Orange", 248, 122, 23},
        {"Orange1", 250, 155, 23},
        {"Orange2", 231, 142, 23},
        {"Orange3", 197, 119, 23},
        {"Orange4", 127, 72, 23},
        {"OrangeRed", 246, 56, 23},
        {"OrangeRed1", 255, 69, 0},
        {"OrangeRed2", 228, 49, 23},
        {"OrangeRed3", 194, 40, 23},
        {"OrangeRed4", 126, 5, 23},
        {"Orchid", 218, 112, 214},
        {"Orchid1", 246, 125, 250},
        {"Orchid2", 228, 115, 231},
        {"Orchid3", 193, 96, 195},
        {"Orchid4", 125, 56, 124},
        {"PaleGoldenrod", 237, 228, 158},
        {"PaleGreen", 121, 216, 103},
        {"PaleGreen1", 154, 255, 154},
        {"PaleGreen2", 144, 238, 144},
        {"PaleGreen3", 124, 205, 124},
        {"PaleGreen4", 78, 124, 65},
        {"PaleTurquoise", 174, 235, 236},
        {"PaleTurquoise1", 187, 255, 255},
        {"PaleTurquoise2", 173, 235, 236},
        {"PaleTurquoise3", 146, 199, 199},
        {"PaleTurquoise4", 102, 139, 139},
        {"PaleVioletRed", 209, 101, 135},
        {"PaleVioletRed1", 247, 120, 161},
        {"PaleVioletRed2", 229, 110, 148},
        {"PaleVioletRed3", 194, 90, 124},
        {"PaleVioletRed4", 126, 53, 77},
        {"PapayaWhip", 254, 236, 207},
        {"PeachPuff", 252, 213, 176},
        {"PeachPuff1", 255, 218, 185},
        {"PeachPuff2", 234, 197, 163},
        {"PeachPuff3", 198, 166, 136},
        {"PeachPuff4", 128, 103, 82},
        {"Peru", 197, 119, 38},
        {"Pink", 250, 175, 190},
        {"Pink1", 255, 181, 197},
        {"Pink2", 231, 161, 176},
        {"Pink3", 196, 135, 147},
        {"Pink4", 127, 82, 93},
        {"Plum", 185, 59, 143},
        {"Plum1", 249, 183, 255},
        {"Plum2", 230, 169, 236},
        {"Plum3", 195, 142, 199},
        {"Plum4", 126, 88, 126},
        {"PowderBlue", 173, 220, 227},
        {"Purple", 142, 53, 239},
        {"Purple1", 137, 59, 255},
        {"Purple2", 127, 56, 236},
        {"Purple3", 108, 45, 199},
        {"Purple4", 70, 27, 126},
        {"Red", 255, 0, 0},
        {"Red1", 246, 34, 23},
        {"Red2", 228, 27, 23},
        {"Red3", 193, 27, 23},
        {"Red4", 126, 5, 23},
        {"RosyBrown", 179, 132, 129},
        {"RosyBrown1", 251, 187, 185},
        {"RosyBrown2", 232, 173, 170},
        {"RosyBrown3", 197, 144, 142},
        {"RosyBrown4", 127, 90, 88},
        {"RoyalBlue", 43, 96, 222},
        {"RoyalBlue1", 48, 110, 255},
        {"RoyalBlue2", 43, 101, 236},
        {"RoyalBlue3", 37, 84, 199},
        {"RoyalBlue4", 21, 49, 126},
        {"SaddleBrown", 126, 49, 23},
        {"Salmon", 225, 139, 107},
        {"Salmon1", 248, 129, 88},
        {"Salmon2", 230, 116, 81},
        {"Salmon3", 195, 98, 65},
        {"Salmon4", 126, 56, 23},
        {"SandyBrown", 238, 154, 77},
        {"SeaGreen", 46, 139, 87},
        {"SeaGreen1", 106, 251, 146},
        {"SeaGreen2", 100, 233, 134},
        {"SeaGreen3", 67, 205, 128},
        {"SeaGreen4", 46, 139, 87},
        {"Seashell", 254, 243, 235},
        {"Seashell1", 255, 245, 238},
        {"Seashell2", 238, 229, 222},
        {"Seashell3", 205, 197, 191},
        {"Seashell4", 139, 134, 130},
        {"Sienna", 160, 82, 45},
        {"Sienna1", 248, 116, 49},
        {"Sienna2", 230, 108, 44},
        {"Sienna3", 195, 88, 23},
        {"Sienna4", 126, 53, 23},
        {"SkyBlue", 102, 152, 255},
        {"SkyBlue1", 130, 202, 255},
        {"SkyBlue2", 121, 186, 236},
        {"SkyBlue3", 101, 158, 199},
        {"SkyBlue4", 65, 98, 126},
        {"SlateBlue", 106, 90, 205},
        {"SlateBlue1", 115, 105, 255},
        {"SlateBlue2", 105, 96, 236},
        {"SlateBlue3", 105, 89, 205},
        {"SlateBlue4", 52, 45, 126},
        {"SlateGray", 101, 115, 131},
        {"SlateGray1", 194, 223, 255},
        {"SlateGray2", 180, 207, 236},
        {"SlateGray3", 152, 175, 199},
        {"SlateGray4", 108, 123, 139},
        {"Snow", 255, 249, 250},
        {"Snow1", 255, 250, 250},
        {"Snow2", 236, 231, 230},
        {"Snow3", 200, 196, 194},
        {"Snow4", 129, 124, 123},
        {"SpringGreen", 0, 255, 127},
        {"SpringGreen1", 94, 251, 110},
        {"SpringGreen2", 0, 238, 118},
        {"SpringGreen3", 0, 205, 102},
        {"SpringGreen4", 0, 139, 69},
        {"SteelBlue", 70, 130, 180},
        {"SteelBlue1", 92, 179, 255},
        {"SteelBlue2", 86, 165, 236},
        {"SteelBlue3", 72, 138, 199},
        {"SteelBlue4", 43, 84, 126},
        {"Tan", 210, 180, 140},
        {"Tan1", 250, 155, 60},
        {"Tan2", 231, 142, 53},
        {"Tan3", 197, 119, 38},
        {"Tan4", 127, 72, 23},
        {"Thistle", 210, 185, 211},
        {"Thistle1", 252, 223, 255},
        {"Thistle2", 233, 207, 236},
        {"Thistle3", 198, 174, 199},
        {"Thistle4", 128, 109, 126},
        {"Tomato", 247, 84, 49},
        {"Tomato1", 255, 99, 71},
        {"Tomato2", 229, 76, 44},
        {"Tomato3", 194, 62, 23},
        {"Tomato4", 126, 34, 23},
        {"Transparent", 21, 5, 23},
        {"Turquoise", 64, 224, 208},
        {"Turquoise1", 0, 245, 255},
        {"Turquoise2", 0, 229, 238},
        {"Turquoise3", 0, 197, 205},
        {"Turquoise4", 0, 134, 139},
        {"Violet", 141, 56, 201},
        {"VioletRed", 208, 32, 144},
        {"VioletRed1", 246, 53, 138},
        {"VioletRed2", 228, 49, 127},
        {"VioletRed3", 193, 40, 105},
        {"VioletRed4", 125, 5, 65},
        {"Wheat", 243, 218, 169},
        {"Wheat1", 254, 228, 177},
        {"Wheat2", 235, 211, 163},
        {"Wheat3", 200, 177, 137},
        {"Wheat4", 129, 111, 84},
        {"White", 255, 255, 255},
        {"WhiteSmoke", 244, 244, 243},
        {"Yellow", 255, 255, 0},
        {"Yellow1", 255, 255, 0},
        {"Yellow2", 238, 233, 23},
        {"Yellow3", 202, 197, 23},
        {"Yellow4", 130, 124, 23},
        {"YellowGreen", 154, 205, 50}
    };
