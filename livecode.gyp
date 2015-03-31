{
	'includes':
	[
		'common.gypi',
	],
	
	'targets':
	[
		{
			'target_name': 'LiveCode-all',
			'type': 'none',
			
			'dependencies':
			[
				# IDE, installer and standalone engines
				'engine/engine.gyp:development',
				'engine/engine.gyp:installer',
				'engine/engine.gyp:standalone',
				
				# Externals
				'revbrowser/revbrowser.gyp:revbrowser',
				'revbrowser/revbrowser.gyp:revbrowser-cefprocess',
				'revdb/revdb.gyp:revdb',
				'revdb/revdb.gyp:dbodbc',
				'revdb/revdb.gyp:dbmysql',
				'revdb/revdb.gyp:dbpostgresql',
				'revdb/revdb.gyp:dbsqlite',
				'revpdfprinter/revpdfprinter.gyp:revpdfprinter',
				'revfont/revfont.gyp:revfont',
				'revmobile/revmobile.gyp:revandroid',
				'revmobile/revmobile.gyp:reviphone',
				'revspeech/revspeech.gyp:revspeech',
				'revvideograbber/revvideograbber.gyp:revvideograbber',
				'revxml/revxml.gyp:revxml',
				'revzip/revzip.gyp:revzip',
			],
		},
	],
}
