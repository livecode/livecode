{
	'includes':
	[
		'../../../engine/engine-sources.gypi',
		'../../../libscript/stdscript-sources.gypi',
	],
	
	'variables':
	{
		'all_syntax_files':
		[
			'<@(engine_syntax_lcb_files)',
            '<@(engine_syntax_only_lcb_files)',
			'<@(stdscript_syntax_lcb_files)',
		],
		
		'gentle_auxiliary_grammar_files':
		[
			'bind.g',
			'check.g',
			'generate.g',
			'support.g',
			'syntax.g',
			'types.g',
		],
		
		'gentle_bootstrap_grammar_file': 'grammar.g',
		
		'template_grammar_file': 'grammar.g',
		
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
		
		'lc-compile_source_files':
		[
			'main.c',
			'operator.c',
			'outputfile.c',
            'outputfile.h',
			'set.c',
			'syntax-gen.c',
			'emit.cpp',
		],
	},
}
