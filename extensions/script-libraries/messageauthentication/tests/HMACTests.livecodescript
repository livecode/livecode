script "HMACTests"
/*
Copyright (C) 2017 LiveCode Ltd.

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

on TestSetup
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/messageauthentication/messageauthentication.livecodescript" \
         into tLibFilename
   
   start using stack tLibFilename
end TestSetup

private command __Test pMessage, pExpected, pDigestType
   local tText, tSecret
   
   switch pMessage
      case "data-28 key-4"
         put "what do ya want for nothing?" into tText
         put "Jefe" into tSecret
         break
      case "data-9 key-16"
         put "Hi There" into tText
         put binaryEncode("H*", "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b") into tSecret
         break
      case "data-9 key-20"
         put "Hi There" into tText
         put binaryEncode("H*", "0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b0b") into tSecret
         break
      case "data-50 key-16"
         put binaryEncode("H*", "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd") into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-50 key-20"
         put binaryEncode("H*", "dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd") into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-50 key-25"
         put binaryEncode("H*", "cdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcdcd") into tText
         put binaryEncode("H*", "0102030405060708090a0b0c0d0e0f10111213141516171819") into tSecret
         break
      case "data-54 key-80"
         put "Test Using Larger Than Block-Size Key - Hash Key First" into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-54 key-131"
         put "Test Using Larger Than Block-Size Key - Hash Key First" into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-54 key-147"
         put "Test Using Larger Than Block-Size Key - Hash Key First" into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-73 key-80"
         put "Test Using Larger Than Block-Size Key and Larger Than One Block-Size Data" into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-152 key-131"
         put "This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm." into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
      case "data-152 key-147"
         put "This is a test using a larger than block-size key and a larger than block-size data. The key needs to be hashed before being used by the HMAC algorithm." into tText
         put binaryEncode("H*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa") into tSecret
         break
   end switch
   
   local tHMAC
   put messageAuthenticationCode(tText, tSecret, pDigestType) into tHMAC
   TestAssert pDigestType & ":" && pMessage, tHMAC is pExpected
end __Test

private command __TestDigestType pDigestType, pTests
   repeat for each key tMessage in pTests
      __Test tMessage, pTests[tMessage], pDigestType
   end repeat
end __TestDigestType

on TestMD5
   local tTests
   put binaryEncode("H*", "9294727a3638bb1c13f48ef8158bfc9d") into tTests["data-9 key-16"]
   put binaryEncode("H*", "750c783e6ab0b503eaa86e310a5db738") into tTests["data-28 key-4"]
   put binaryEncode("H*", "56be34521d144c88dbb8c733f0e8b3f6") into tTests["data-50 key-16"]
   put binaryEncode("H*", "697eaf0aca3a3aea3a75164746ffaa79") into tTests["data-50 key-25"]
   put binaryEncode("H*", "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd") into tTests["data-54 key-80"]
   put binaryEncode("H*", "6f630fad67cda0ee1fb1f562db3aa53e") into tTests["data-73 key-80"]
   
   __TestDigestType "HMAC-MD5", tTests
end TestMD5

on TestSHA_1
   local tTests
   put binaryEncode("H*", "b617318655057264e28bc0b6fb378c8ef146be00") into tTests["data-9 key-20"]
   put binaryEncode("H*", "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79") into tTests["data-28 key-4"]
   put binaryEncode("H*", "125d7342b9ac11cd91a39af48aa17b4f63f175d3") into tTests["data-50 key-20"]
   put binaryEncode("H*", "4c9007f4026250c6bc8414f9bf50c86c2d7235da") into tTests["data-50 key-25"]
   put binaryEncode("H*", "aa4ae5e15272d00e95705637ce8a3b55ed402112") into tTests["data-54 key-80"]
   put binaryEncode("H*", "e8e99d0f45237d786d6bbaa7965c7808bbff1a91") into tTests["data-73 key-80"]
   
   __TestDigestType "HMAC-SHA-1", tTests
end TestSHA_1

on TestSHA_224
   local tTests
   put binaryEncode("H*", "a30e01098bc6dbbf45690f3a7e9e6d0f8bbea2a39e6148008fd05e44") into tTests["data-28 key-4"]
   put binaryEncode("H*", "896fb1128abbdf196832107cd49df33f47b4b1169912ba4f53684b22") into tTests["data-9 key-20"]
   put binaryEncode("H*", "7fb3cb3588c6c1f6ffa9694d7d6ad2649365b0c1f65d69d1ec8333ea") into tTests["data-50 key-20"]
   put binaryEncode("H*", "6c11506874013cac6a2abc1bb382627cec6a90d86efc012de7afec5a") into tTests["data-50 key-25"]
   put binaryEncode("H*", "95e9a0db962095adaebe9b2d6f0dbce2d499f112f2d2b7273fa6870e") into tTests["data-54 key-131"]
   put binaryEncode("H*", "3a854166ac5d9f023f54d517d0b39dbd946770db9c2b95c9f6f565d1") into tTests["data-152 key-131"]
   
   __TestDigestType "HMAC-SHA-224", tTests
end TestSHA_224

on TestSHA_256
   local tTests
   put binaryEncode("H*", "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843") into tTests["data-28 key-4"]
   put binaryEncode("H*", "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7") into tTests["data-9 key-20"]
   put binaryEncode("H*", "773ea91e36800e46854db8ebd09181a72959098b3ef8c122d9635514ced565fe") into tTests["data-50 key-20"]
   put binaryEncode("H*", "82558a389a443c0ea4cc819899f2083a85f0faa3e578f8077a2e3ff46729665b") into tTests["data-50 key-25"]
   put binaryEncode("H*", "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54") into tTests["data-54 key-131"]
   put binaryEncode("H*", "9b09ffa71b942fcb27635fbcd5b0e944bfdc63644f0713938a7f51535c3a35e2") into tTests["data-152 key-131"]
   
   __TestDigestType "HMAC-SHA-256", tTests
end TestSHA_256

on TestSHA_384
   local tTests
   put binaryEncode("H*", "af45d2e376484031617f78d2b58a6b1b9c7ef464f5a01b47e42ec3736322445e8e2240ca5e69e2c78b3239ecfab21649") into tTests["data-28 key-4"]
   put binaryEncode("H*", "afd03944d84895626b0825f4ab46907f15f9dadbe4101ec682aa034c7cebc59cfaea9ea9076ede7f4af152e8b2fa9cb6") into tTests["data-9 key-20"]
   put binaryEncode("H*", "88062608d3e6ad8a0aa2ace014c8a86f0aa635d947ac9febe83ef4e55966144b2a5ab39dc13814b94e3ab6e101a34f27") into tTests["data-50 key-20"]
   put binaryEncode("H*", "3e8a69b7783c25851933ab6290af6ca77a9981480850009cc5577c6e1f573b4e6801dd23c4a7d679ccf8a386c674cffb") into tTests["data-50 key-25"]
   put binaryEncode("H*", "4ece084485813e9088d2c63a041bc5b44f9ef1012a2b588f3cd11f05033ac4c60c2ef6ab4030fe8296248df163f44952") into tTests["data-54 key-131"]
   put binaryEncode("H*", "6617178e941f020d351e2f254e8fd32c602420feb0b8fb9adccebb82461e99c5a678cc31e799176d3860e6110c46523e") into tTests["data-152 key-131"]
   
   __TestDigestType "HMAC-SHA-384", tTests
end TestSHA_384

on TestSHA_512
   local tTests
   put binaryEncode("H*", "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737") into tTests["data-28 key-4"]
   put binaryEncode("H*", "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854") into tTests["data-9 key-20"]
   put binaryEncode("H*", "fa73b0089d56a284efb0f0756c890be9b1b5dbdd8ee81a3655f83e33b2279d39bf3e848279a722c806b485a47e67c807b946a337bee8942674278859e13292fb") into tTests["data-50 key-20"]
   put binaryEncode("H*", "b0ba465637458c6990e5a8c5f61d4af7e576d97ff94b872de76f8050361ee3dba91ca5c11aa25eb4d679275cc5788063a5f19741120c4f2de2adebeb10a298dd") into tTests["data-50 key-25"]
   put binaryEncode("H*", "80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598") into tTests["data-54 key-131"]
   put binaryEncode("H*", "e37b6a775dc87dbaa4dfa9f96e5e3ffddebd71f8867289865df5a32d20cdc944b6022cac3c4982b10d5eeb55c3e4de15134676fb6de0446065c97440fa8c6a58") into tTests["data-152 key-131"]
   
   __TestDigestType "HMAC-SHA-512", tTests
end TestSHA_512

on TestSHA3_224
   local tTests
   put binaryEncode("H*", "7fdb8dd88bd2f60d1b798634ad386811c2cfc85bfaf5d52bbace5e66") into tTests["data-28 key-4"]
   put binaryEncode("H*", "3b16546bbc7be2706a031dcafd56373d9884367641d8c59af3c860f7") into tTests["data-9 key-20"]
   put binaryEncode("H*", "676cfc7d16153638780390692be142d2df7ce924b909c0c08dbfdc1a") into tTests["data-50 key-20"]
   put binaryEncode("H*", "a9d7685a19c4e0dbd9df2556cc8a7d2a7733b67625ce594c78270eeb") into tTests["data-50 key-25"]
   put binaryEncode("H*", "b4a1f04c00287a9b7f6075b313d279b833bc8f75124352d05fb9995f") into tTests["data-54 key-131"]
   put binaryEncode("H*", "05d8cd6d00faea8d1eb68ade28730bbd3cbab6929f0a086b29cd62a0") into tTests["data-152 key-131"]
   put binaryEncode("H*", "b96d730c148c2daad8649d83defaa3719738d34775397b7571c38515") into tTests["data-54 key-147"]
   put binaryEncode("H*", "c79c9b093424e588a9878bbcb089e018270096e9b4b1a9e8220c866a") into tTests["data-152 key-147"]
   
   __TestDigestType "HMAC-SHA3-224", tTests
end TestSHA3_224

on TestSHA3_256
   local tTests
   put binaryEncode("H*", "c7d4072e788877ae3596bbb0da73b887c9171f93095b294ae857fbe2645e1ba5") into tTests["data-28 key-4"]
   put binaryEncode("H*", "ba85192310dffa96e2a3a40e69774351140bb7185e1202cdcc917589f95e16bb") into tTests["data-9 key-20"]
   put binaryEncode("H*", "84ec79124a27107865cedd8bd82da9965e5ed8c37b0ac98005a7f39ed58a4207") into tTests["data-50 key-20"]
   put binaryEncode("H*", "57366a45e2305321a4bc5aa5fe2ef8a921f6af8273d7fe7be6cfedb3f0aea6d7") into tTests["data-50 key-25"]
   put binaryEncode("H*", "ed73a374b96c005235f948032f09674a58c0ce555cfc1f223b02356560312c3b") into tTests["data-54 key-131"]
   put binaryEncode("H*", "65c5b06d4c3de32a7aef8763261e49adb6e2293ec8e7c61e8de61701fc63e123") into tTests["data-152 key-131"]
   put binaryEncode("H*", "a6072f86de52b38bb349fe84cd6d97fb6a37c4c0f62aae93981193a7229d3467") into tTests["data-54 key-147"]
   put binaryEncode("H*", "e6a36d9b915f86a093cac7d110e9e04cf1d6100d30475509c2475f571b758b5a") into tTests["data-152 key-147"]
   
   __TestDigestType "HMAC-SHA3-256", tTests
end TestSHA3_256

on TestSHA3_384
   local tTests
   put binaryEncode("H*", "f1101f8cbf9766fd6764d2ed61903f21ca9b18f57cf3e1a23ca13508a93243ce48c045dc007f26a21b3f5e0e9df4c20a") into tTests["data-28 key-4"]
   put binaryEncode("H*", "68d2dcf7fd4ddd0a2240c8a437305f61fb7334cfb5d0226e1bc27dc10a2e723a20d370b47743130e26ac7e3d532886bd") into tTests["data-9 key-20"]
   put binaryEncode("H*", "275cd0e661bb8b151c64d288f1f782fb91a8abd56858d72babb2d476f0458373b41b6ab5bf174bec422e53fc3135ac6e") into tTests["data-50 key-20"]
   put binaryEncode("H*", "3a5d7a879702c086bc96d1dd8aa15d9c46446b95521311c606fdc4e308f4b984da2d0f9449b3ba8425ec7fb8c31bc136") into tTests["data-50 key-25"]
   put binaryEncode("H*", "0fc19513bf6bd878037016706a0e57bc528139836b9a42c3d419e498e0e1fb9616fd669138d33a1105e07c72b6953bcc") into tTests["data-54 key-131"]
   put binaryEncode("H*", "026fdf6b50741e373899c9f7d5406d4eb09fc6665636fc1a530029ddf5cf3ca5a900edce01f5f61e2f408cdf2fd3e7e8") into tTests["data-152 key-131"]
   put binaryEncode("H*", "713dff0302c85086ec5ad0768dd65a13ddd79068d8d4c6212b712e41649449111480230044185a99103ed82004ddbfcc") into tTests["data-54 key-147"]
   put binaryEncode("H*", "cad18a8ff6c4cc3ad487b95f9769e9b61c062aefd6952569e6e6421897054cfc70b5fdc6605c18457112fc6aaad45585") into tTests["data-152 key-147"]
   
   __TestDigestType "HMAC-SHA3-384", tTests
end TestSHA3_384

on TestSHA3_512
   local tTests
   put binaryEncode("H*", "5a4bfeab6166427c7a3647b747292b8384537cdb89afb3bf5665e4c5e709350b287baec921fd7ca0ee7a0c31d022a95e1fc92ba9d77df883960275beb4e62024") into tTests["data-28 key-4"]
   put binaryEncode("H*", "eb3fbd4b2eaab8f5c504bd3a41465aacec15770a7cabac531e482f860b5ec7ba47ccb2c6f2afce8f88d22b6dc61380f23a668fd3888bb80537c0a0b86407689e") into tTests["data-9 key-20"]
   put binaryEncode("H*", "309e99f9ec075ec6c6d475eda1180687fcf1531195802a99b5677449a8625182851cb332afb6a89c411325fbcbcd42afcb7b6e5aab7ea42c660f97fd8584bf03") into tTests["data-50 key-20"]
   put binaryEncode("H*", "b27eab1d6e8d87461c29f7f5739dd58e98aa35f8e823ad38c5492a2088fa0281993bbfff9a0e9c6bf121ae9ec9bb09d84a5ebac817182ea974673fb133ca0d1d") into tTests["data-50 key-25"]
   put binaryEncode("H*", "00f751a9e50695b090ed6911a4b65524951cdc15a73a5d58bb55215ea2cd839ac79d2b44a39bafab27e83fde9e11f6340b11d991b1b91bf2eee7fc872426c3a4") into tTests["data-54 key-131"]
   put binaryEncode("H*", "38a456a004bd10d32c9ab8336684112862c3db61adcca31829355eaf46fd5c73d06a1f0d13fec9a652fb3811b577b1b1d1b9789f97ae5b83c6f44dfcf1d67eba") into tTests["data-152 key-131"]
   put binaryEncode("H*", "b14835c819a290efb010ace6d8568dc6b84de60bc49b004c3b13eda763589451e5dd74292884d1bdce64e6b919dd61dc9c56a282a81c0bd14f1f365b49b83a5b") into tTests["data-54 key-147"]
   put binaryEncode("H*", "dc030ee7887034f32cf402df34622f311f3e6cf04860c6bbd7fa488674782b4659fdbdf3fd877852885cfe6e22185fe7b2ee952043629bc9d5f3298a41d02c66") into tTests["data-152 key-147"]
   
   __TestDigestType "HMAC-SHA3-512", tTests
end TestSHA3_512

