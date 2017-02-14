{
	'includes':
	[
		'../../../engine/engine-sources.gypi',
	],
	
	'variables':
	{
		'all_syntax_files':
		[
		],
		
		'gentle_auxiliary_grammar_files':
		[
			'bind.g',
			'check.g',
			'output.g',
			'generate.g',
			'support.g',
			'types.g',
		],
		
		'gentle_grammar_file': 'grammar.g',
		
		'reflex_source_files':
		[
			'COMMENTS.b',
			'DOUBLE_LITERAL.t',
			'END_OF_UNIT.t',
			'ILLEGAL.b',
			'INTEGER_LITERAL.t',
			'LAYOUT.b',
			'LEXDEF.b',
			'LEXFUNC.b',
			'LITBLOCK.b',
			'NAME_LITERAL.t',
			'NEXT_UNIT.t',
			'SEPARATOR.t',
			'SETPOS.b',
			'STRING_LITERAL.t',
			'YYSTYPE.b',
			'YYWRAP.b',
		],
		
		'lc-compile-ffi-java_source_files':
		[
			'literal.c',
			'main.c',
			'outputfile.c',
            'outputfile.h',
			'output.cpp',
		],
	},
}
