/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

package com.runrev.android.billing;

import android.util.*;

import java.security.*;
import java.security.spec.*;
import java.util.*;

public class Security
{
	private static final String TAG = "Security";
	
	private static final SecureRandom RANDOM = new SecureRandom();
	
	private static HashSet<Long> sKnownNonces = new HashSet<Long>();
	
	private static byte[] sPublicKey = null;
	
	public static long generateNonce()
	{
		long nonce = RANDOM.nextLong();
		sKnownNonces.add(nonce);
		return nonce;
	}
	
	public static void removeNonce(long nonce)
	{
		sKnownNonces.remove(nonce);
	}
	
	public static boolean isNonceKnown(long nonce)
	{
		return sKnownNonces.contains(nonce);
	}
	
	public static void setPublicKey(byte[] pKey)
	{
		sPublicKey = pKey;
	}
	
	// set base64-encoded key string
	public static boolean setPublicKey(String pKey)
	{
		if (pKey == null)
		{
			sPublicKey = null;
			return true;
		}
		
		try
		{
			byte[] tKey = Base64.decode(pKey, Base64.DEFAULT);
			setPublicKey(tKey);
			return true;
		}
		catch (IllegalArgumentException e)
		{
			// couldn't decode key string
		}
		return false;
	}
	
	public static PublicKey generatePublicKey(byte[] publicKeyBytes)
	{
		try
		{
			KeyFactory keyFactory = KeyFactory.getInstance("RSA");
			return keyFactory.generatePublic(new X509EncodedKeySpec(publicKeyBytes));
		}
		catch (NoSuchAlgorithmException e)
		{
			throw new RuntimeException(e);
		}
		catch (InvalidKeySpecException e)
		{
			throw new IllegalArgumentException(e);
		}
	}
	
	public static boolean verify(PublicKey publicKey, String signedData, String signature)
	{
		Signature sig;
		try
		{
			sig = Signature.getInstance("SHA1withRSA");
			sig.initVerify(publicKey);
			sig.update(signedData.getBytes());
			return sig.verify(Base64.decode(signature, Base64.DEFAULT));
		}
		catch (NoSuchAlgorithmException e)
		{
		}
		catch (InvalidKeyException e)
		{
		}
		catch (SignatureException e)
		{
		}
		return false;
	}
	
	public static boolean verify(byte[] publicKeyBytes, String signedData, String signature)
	{
		return verify(generatePublicKey(publicKeyBytes), signedData, signature);
	}
	
	public static boolean verify(String signedData, String signature)
	{
		if (sPublicKey == null)
			return false;
		return verify(sPublicKey, signedData, signature);
	}
}
