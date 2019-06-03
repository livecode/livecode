{
	'includes':
	[
		'../common.gypi',
	],

	'target_defaults':
	{
		'variables':
		{
			'openssl_headers':
			[
				'include/openssl/aes.h',
				'include/openssl/asn1.h',
				'include/openssl/asn1_mac.h',
				'include/openssl/asn1t.h',
				'include/openssl/bio.h',
				'include/openssl/blowfish.h',
				'include/openssl/bn.h',
				'include/openssl/buffer.h',
				'include/openssl/camellia.h',
				'include/openssl/cast.h',
				'include/openssl/cmac.h',
				'include/openssl/cms.h',
				'include/openssl/comp.h',
				'include/openssl/conf.h',
				'include/openssl/conf_api.h',
				'include/openssl/crypto.h',
				'include/openssl/des.h',
				'include/openssl/des_old.h',
				'include/openssl/dh.a',
				'include/openssl/dsa.h',
				'include/openssl/dso.h',
				'include/openssl/dtls1.h',
				'include/openssl/e_os2.h',
				'include/openssl/ebcdic.h',
				'include/openssl/ec.h',
				'include/openssl/ecdh.h',
				'include/openssl/ecsda.h',
				'include/openssl/engine.h',
				'include/openssl/err.h',
				'include/openssl/evp.h',
				'include/openssl/hmac.h',
				'include/openssl/kbr5_asn.h',
				'include/openssl/kssl.h',
				'include/openssl/lhash.h',
				'include/openssl/md4.h',
				'include/openssl/md5.h',
				'include/openssl/mdc2.h',
				'include/openssl/modes.h',
				'include/openssl/obj_mac.h',
				'include/openssl/objects.h',
				'include/openssl/ocsp.h',
				'include/openssl/opensslconf.h',
				'include/openssl/opensslv.h',
				'include/openssl/ossl_typ.h',
				'include/openssl/pem.h',
				'include/openssl/pem2.h',
				'include/openssl/pkcs7.h',
				'include/openssl/pkcs12.h',
				'include/openssl/pqueue.h',
				'include/openssl/rand.h',
				'include/openssl/rc2.h',
				'include/openssl/rc4.h',
				'include/openssl/ripemd.h',
				'include/openssl/rsa.h',
				'include/openssl/safestack.h',
				'include/openssl/seed.h',
				'include/openssl/sha.h',
				'include/openssl/srp.h',
				'include/openssl/srtp.h',
				'include/openssl/ssl.h',
				'include/openssl/ssl2.h',
				'include/openssl/ssl3.h',
				'include/openssl/ssl23.h',
				'include/openssl/stack.h',
				'include/openssl/symhacks.h',
				'include/openssl/tls1.h',
				'include/openssl/ts.h',
				'include/openssl/txt_db.h',
				'include/openssl/ui.h',
				'include/openssl/ui_compat.h',
				'include/openssl/whrlpool.h',
				'include/openssl/x509.h',
				'include/openssl/x509_vfy.h',
				'include/openssl/x509_v3.h',
			],
			
			'icu_headers':
			[
				'include/layout/LayoutEngine.h',
				'include/layout/LEFontInstance.h',
				'include/layout/LEGlyphFilter.h',
				'include/layout/LEGlyphStorage.h',
				'include/layout/LEInsertionList.h',
				'include/layout/LELanguages.h',
				'include/layout/LEScripts.h',
				'include/layout/LESwaps.h',
				'include/layout/LETableReference.h',
				'include/layout/LETypes.h',
				'include/layout/loengine.h',
				'include/layout/ParagraphLayout.h',
				'include/layout/playout.h',
				'include/layout/plruns.h',
				'include/layout/RunArrays.h',
				
				'include/unicode/alphaindex.h',
				'include/unicode/appendable.h',
				'include/unicode/basictz.h',
				'include/unicode/brkiter.h',
				'include/unicode/bytestream.h',
				'include/unicode/bytestrie.h',
				'include/unicode/coleitr.h',
				'include/unicode/coll.h',
				'include/unicode/compactdecimalformat.h',
				'include/unicode/curramt.h',
				'include/unicode/currpinf.h',
				'include/unicode/currunit.h',
				'include/unicode/dtfmtsym.h',
				'include/unicode/dtintrv.h',
				'include/unicode/dtitvfmt.h',
				'include/unicode/dtitvinf.h',
				'include/unicode/dtpngen.h',
				'include/unicode/dtrule.h',
				'include/unicode/fpositer.h',
				'include/unicode/gender.h',
				'include/unicode/gregocal.h',
				'include/unicode/icudataver.h',
				'include/unicode/icuplug.h',
				'include/unicode/idna.h',
				'include/unicode/measunit.h',
				'include/unicode/measure.h',
				'include/unicode/messagepattern.h',
				'include/unicode/msgfmt.h',
				'include/unicode/normalizer2.h',
				'include/unicode/normlzr.h',
				'include/unicode/plurfmt.h',
				'include/unicode/plurrule.h',
				'include/unicode/ptypes.h',
				'include/unicode/putil.h',
				'include/unicode/rbbi.h',
				'include/unicode/rbnf.h',
				'include/unicode/schriter.h',
				'include/unicode/search.h',
				'include/unicode/selfmt.h',
				'include/unicode/simpletz.h',
				'include/unicode/smpdtfmt.h',
				'include/unicode/sortkey.h',
				'include/unicode/symtable.h',
				'include/unicode/tblcoll.h',
				'include/unicode/timezone.h',
				'include/unicode/tmunit.h',
				'include/unicode/tmutamt.h',
				'include/unicode/tmutfmt.h',
				'include/unicode/ubidi.h',
				'include/unicode/ubrk.h',
				'include/unicode/ucal.h',
				'include/unicode/ucasemap.h',
				'include/unicode/ucat.h',
				'include/unicode/uchar.h',
				'include/unicode/ucnv_err.h',
				'include/unicode/ucnv.h',
				'include/unicode/ucnvsel.h',
				'include/unicode/ucol.h',
				'include/unicode/ucoleitr.h',
				'include/unicode/uconfig.h',
				'include/unicode/udatapg.h',
				'include/unicode/udisplaycontext.h',
				'include/unicode/uenum.h',
				'include/unicode/uformattable.h',
				'include/unicode/ugender.h',
				'include/unicode/uidna.h',
				'include/unicode/umisc.h',
				'include/unicode/umsg.h',
				'include/unicode/unifilt.h',
				'include/unicode/unifunct.h',
				'include/unicode/unimatch.h',
				'include/unicode/unirepl.h',
				'include/unicode/unumsys.h',
				'include/unicode/uobject.h',
				'include/unicode/upluralrules.h',
				'include/unicode/uregex.h',
				'include/unicode/uregion.h',
				'include/unicode/urename.h',
				'include/unicode/usetiter.h',
				'include/unicode/ushape.h',
				'include/unicode/uspoof.h',
				'include/unicode/usprep.h',
				'include/unicode/ustdio.h',
				'include/unicode/ustream.h',
				'include/unicode/utf8.h',
				'include/unicode/utf16.h',
				'include/unicode/utf32.h',
				'include/unicode/utmscale.h',
				'include/unicode/utrace.h',
				'include/unicode/utrans.h',
			],
		},
	},

	'targets':
	[
		{
			'target_name': 'fetch-all',
			'type': 'none',
			
			'dependencies':
			[
				'fetch-android',
				'fetch-linux',
				'fetch-mac',
				'fetch-win',
				'fetch-ios',
				'fetch-emscripten',
			],
		},
		{
			'target_name': 'fetch',
			'type': 'none',
			
			'toolsets': ['host','target'],
			
			'variables':
			{
				'conditions':
				[
					[
						'_toolset == "host"',
						{
							'fetch_os': '<(host_os)',
						},
						{
							'fetch_os': '<(OS)',
						},
					],
				],
			},
			
			'conditions':
			[
				[
					'fetch_os == "android"',
					{
						'dependencies':
						[
							'fetch-android#target',
						],
					},
				],
				[
					'fetch_os == "linux"',
					{
						'dependencies':
						[
							'fetch-linux#target',
						],
					},
				],
				[
					'fetch_os == "mac"',
					{
						'dependencies':
						[
							'fetch-mac#target',
						],
					},
				],
				[
					'fetch_os == "win"',
					{
						'dependencies':
						[
							'fetch-win#target',
						],
					},
				],
				[
					'fetch_os == "ios"',
					{
						'dependencies':
						[
							'fetch-ios#target',
						],
					},
				],
				[
					'fetch_os == "emscripten"',
					{
						'dependencies':
						[
							'fetch-emscripten#target',
						],
					}
				],
			],
		},
		{
			'target_name': 'fetch-android',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Android',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/android/>(target_arch)',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'android',
						'>(target_arch)',
					],
				},
			],
		},
		{
			'target_name': 'fetch-linux',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Linux',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'bin/linux',
						'lib/linux',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'linux',
						'<(host_arch)',
					],
				},
			],
		},
		{
			'target_name': 'fetch-mac',
			'type': 'none',
			
			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for OSX',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'bin/mac',
						'lib/mac',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'mac',
					],
				},
			],
		},
		{
			'target_name': 'fetch-win',
			'type': 'none',

			'variables':
			{
				'conditions':
				[
					[
						'target_arch == "x64"',
						{
							'fetch_arch': 'x86_64',
						},
						{
							'fetch_arch': 'x86',
						},
					],
				],
			},

			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Windows',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'bin/win32/>(fetch_arch)',
						'lib/win32/>(fetch_arch)',
                        'unpacked',
					],
					
					'action':
					[
						'call',
						'../util/invoke-unix.bat',
						'./fetch-libraries.sh',
						# Ensure gyp does not treat these parameters as paths
						'$(not_a_real_variable)win32',
						'$(not_a_real_variable)>(fetch_arch)',
					],
				},
			],
		},
		{
			'target_name': 'fetch-ios',
			'type': 'none',

			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for iOS',
					
					'inputs':
					[
						'fetch-libraries.sh',
					],
					
					'outputs':
					[
						'lib/ios',
					],
					
					'action':
					[
						'./fetch-libraries.sh',
						'ios',
					],
				},
			],
		},
		{
			'target_name': 'fetch-emscripten',
			'type': 'none',

			'actions':
			[
				{
					'action_name': 'fetch',
					'message': 'Fetching prebuilt libraries for Emscripten',

					'inputs':
					[
						'fetch-libraries.sh',
					],

					'outputs':
					[
						'lib/emscripten/js',
					],

					'action':
					[
						'./fetch-libraries.sh',
						'emscripten',
					],
				},
			],
		},
	],
}
