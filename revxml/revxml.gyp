{
	'includes':
	[
		'../common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'revxml',
			'type': 'loadable_module',
			'mac_bundle': 1,
			'product_prefix': '',
			
			'dependencies':
			[
				'../libexternal/libexternal.gyp:libExternal',
				'../thirdparty/libxml/libxml.gyp:libxml',
				'../thirdparty/libxslt/libxslt.gyp:libxslt',
				'../thirdparty/libz/libz.gyp:libz',
			],
			
			'include_dirs':
			[
				'src',
			],
			
			'sources':
			[
				'src/cxml.h',
				'src/revxml.h',
				'src/revxml.cpp',
				'src/xmlattribute.cpp',
				'src/xmldoc.cpp',
				'src/xmlelement.cpp',
			],
			
			'xcode_settings':
			{
				'INFOPLIST_FILE': 'rsrc/revxml-Info.plist',
				'EXPORTED_SYMBOLS_FILE': 'revxml.exports',
			},
			
			'variables':
			{
				'ios_external_symbols': [ '_getXtable' ],
			},
		},
	],
}
