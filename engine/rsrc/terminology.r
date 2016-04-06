#include <Carbon/Carbon.r>

#define Reserved8   reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
#define Reserved12  Reserved8, reserved, reserved, reserved, reserved
#define Reserved13  Reserved12, reserved
#define dp_none__   noParams, "", directParamOptional, singleItem, notEnumerated, Reserved13
#define reply_none__   noReply, "", replyOptional, singleItem, notEnumerated, Reserved13
#define synonym_verb__ reply_none__, dp_none__, { }
#define plural__    "", {"", kAESpecialClassProperties, cType, "", reserved, singleItem, notEnumerated, readOnly, Reserved8, noApostrophe, notFeminine, notMasculine, plural}, {}

resource 'aete' (0, "LiveCode Terminology") {
	0x1,  // major version
	0x0,  // minor version
	english,
	roman,
	{
		"LiveCode Miscellaneous Commands",
		"Miscellaneous commands",
		'REVO',
		1,
		1,
		{
			/* Events */

			"do script",
			"Execute LiveCode script",
			'misc', 'dosc',
			'TEXT',
			"Return value",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"Script to execute",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"evaluate",
			"Evaluate LiveCode expression",
			'misc', 'eval',
			'TEXT',
			"Value of expression",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"Expression to evaluate",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			}
		},
		{
			/* Classes */

		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
