package com.runrev.external;

import java.lang.*;
import android.app.*;
import android.view.*;
import android.content.*;
import com.runrev.android.EngineApi;

public class LC
{
	public static final int USE_CASE_SENSITIVE = 1 << 30;
	public static final int USE_CONVERT_OCTALS = 1 << 28;
	public static final int USE_NUMBER_FORMAT_DECIMAL = 0 << 26;
	public static final int USE_NUMBER_FORMAT_SCIENTIFIC = 1 << 26;
	public static final int USE_NUMBER_FORMAT_COMPACT = 2 << 26;

	public class Error extends RuntimeException
	{
		private int m_code;
		
		public Error(int p_code)
		{
			m_code = p_code;
		}
		
		public int getCode()
		{
			return m_code;
		}
	};

	/*public static class Array
	{
		private long m_pointer;
		
		public Array()
		{
			m_pointer = ArrayCreate(0);
		}
		
		public void Release()
		{
			if (m_pointer == 0)
				return;
			
			ArrayRelease(m_pointer);
			m_pointer = 0;
		}
		
		//////////
		
		public int CountKeys()
		{
			Check();
			return ArrayCountKeys(m_pointer, 0, null);
		}
		
		public String[] ListKeys()
		{
			Check();
			return ArrayListKeys(m_pointer, 0, null);
		}
		
		public void RemoveKeys()
		{
			Check();
			ArrayRemoveKeys(m_pointer, 0, null);
		}
		
		//////////
		
		boolean HasKey(String p_key)
		{
			Check();
			return ArrayHasKey(m_pointer, 0, null, p_key);
		}
		
		boolean HasExactKey(String p_key)
		{
			Check();
			return ArrayHasKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		java.lang.Object FetchKey(String p_key)
		{
			Check();
			return ArrayFetchKey(m_pointer, 0, null, p_key);
		}
		
		java.lang.Object FetchExactKey(String p_key)
		{
			Check();
			return ArrayFetchKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		void StoreKey(String p_key, Object p_value)
		{
			Check();
			ArrayStoreKey(m_pointer, 0, null, p_key, p_value);
		}
		
		void StoreExactKey(String p_key, Object p_value)
		{
			Check();
			ArrayStoreKey(m_pointer, USE_CASE_SENSITIVE, null, p_key, p_value);
		}
		
		void RemoveKey(String p_key)
		{
			Check();
			ArrayRemoveKey(m_pointer, 0, null, p_key);
		}
		
		void RemoveExactKey(String p_key)
		{
			Check();
			ArrayRemoveKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		//////////
		
		protected void finalize() throws java.lang.Throwable
		{
			Release();
			super.finalize();
		}
		
		//////////
		
		private Array(long p_pointer)
		{
			m_pointer = p_pointer;
		}
		
		private void Check()
		{
			if (m_pointer == 0)
				throw new RuntimeException();
		}
		
		//////////
		
	};*/
	
	public static class Object
	{
		private long m_pointer;
		
		public Object(String p_chunk)
		{
			m_pointer = __ObjectResolve(p_chunk);
		}
		
		public void Release()
		{
			if (m_pointer == 0)
				return;
				
			__ObjectRelease(m_pointer);
				
			m_pointer = 0;
		}
		
		//////////
		
		public boolean Exists()
		{
			Check();
			return true;
		}
			
		//////////
		
		public void Send(String p_message, java.lang.Object... p_arguments)
		{
			Check();
			__ObjectSend(m_pointer, p_message, p_arguments);
		}
		
		public void Post(String p_message, java.lang.Object... p_arguments)
		{
			Check();
			__ObjectPost(m_pointer, p_message, p_arguments);
		}
		
		//////////
		
		private Object(long p_pointer)
		{
			m_pointer = p_pointer;
		}
		
		private void Check()
		{
			if (m_pointer == 0)
				throw new RuntimeException();
		}
		
		//////////
	};
	
	public static class Wait
	{
		private long m_pointer;
		
		public Wait(boolean p_allow_dispatch)
		{
			if (p_allow_dispatch)
				m_pointer = __WaitCreate(1);
			else
				m_pointer = __WaitCreate(0);
		}
		
		public void Release()
		{
			if (m_pointer == 0)
				return;
			
			__WaitRelease(m_pointer);
			
			m_pointer = 0;
		}
		
		public boolean IsRunning()
		{
			Check();
			return __WaitIsRunning(m_pointer);
		}
		
		public void Run()
		{
			Check();
			__WaitRun(m_pointer);
		}
		
		public void Break()
		{
			Check();
			__WaitBreak(m_pointer);
		}
		
		public void Reset()
		{
			Check();
			__WaitReset(m_pointer);
		}
		
		private void Check()
		{
			if (m_pointer == 0)
				throw new RuntimeException();
		}
		
		protected void finalize() throws java.lang.Throwable
		{
			Release();
			super.finalize();
		}
	};
	
	public static Object ContextMe()
	{
		return new Object(__ContextMe());
	}
	
	public static Object ContextTarget()
	{
		return new Object(__ContextTarget());
	}
	
	///////////
	
	public static Activity InterfaceQueryActivity()
	{
		return getEngineApi() . getActivity();
	}
	
	public static ViewGroup InterfaceQueryContainer()
	{
		return getEngineApi() . getContainer();
	}
    
    public static double InterfaceQueryViewScale()
    {
        return __InterfaceQueryViewScale();
    }
	
	//////////
	
	public interface ActivityResultCallback
	{
		public abstract void handleActivityResult(int resultCode, Intent data);
	};

	public static void ActivityRun(Intent p_intent, ActivityResultCallback p_callback)
	{
		final ActivityResultCallback t_callback = p_callback;
		getEngineApi() . runActivity(p_intent, new EngineApi.ActivityResultCallback() {
			public void handleActivityResult(int resultCode, Intent data)
			{
				t_callback . handleActivityResult(resultCode, data);
			}
		});
	}
	
	//////////
	
	public static void RunOnSystemThread(Runnable p_callback)
	{
		__RunOnSystemThread(p_callback);
	}
	
	//////////
	
	private static EngineApi s_engine_api = null;
	
	private static EngineApi getEngineApi()
	{
		if (s_engine_api == null)
			s_engine_api = __InterfaceQueryEngine();
		return s_engine_api;
	}
	
	//////////
	
	/*private static native long ArrayCreate(int options);
	private static native void ArrayRetain(long array);
	private static native void ArrayRelease(long array);
	private static native int ArrayCountKeys(long array, int options, String[] path);
	private static native String[] ArrayListKeys(long array, int options, String[] path);
	private static native void ArrayRemoveKeys(long array, int options, String[] path);
	
	private static native boolean ArrayHasKey(long array, int options, String[] path, String key);
	private static native java.lang.Object ArrayFetchKey(long array, int options, String[] path, String key);
	private static native void ArrayStoreKey(long array, int options, String[] path, String key, java.lang.Object value);
	private static native void ArrayRemoveKey(long array, int options, String[] path, String key);*/
	
	private static native long __ObjectResolve(String chunk);
	private static native void __ObjectRetain(long object);
	private static native void __ObjectRelease(long object);
	private static native boolean __ObjectExists(long object);
	private static native void __ObjectSend(long object, String message, java.lang.Object[] arguments);
	private static native void __ObjectPost(long object, String message, java.lang.Object[] arguments);
	/*private static native java.lang.Object __ObjectGet(long object, int options, String property, String key);
	private static native void __ObjectSet(long object, int options, String property, String key);*/
	
	private static native long __ContextMe();
	private static native long __ContextTarget();
	/*private static native long __ContextDefaultStack();
	private static native long __ContextDefaultCard();
	public static native boolean __ContextCaseSensitive();
	public static native boolean __ContextConvertOctals();
	public static native boolean __ContextWholeMatches();
	private static native Object __ContextItemDelimiter(int options);
	private static native Object __ContextLineDelimiter(int options);
	private static native Object __ContextRowDelimiter(int options);
	private static native Object __ContextColumnDelimiter(int options);
	private static native Object __ContextResult(int options);
	private static native Object __ContextIt(int options);
	private static native Object __ContextEvaluate(String expression, int options);
	private static native Object __ContextExecute(String statements, int options);*/
	
	private static native long __WaitCreate(int options);
	//private static native long __WaitRetain(long wait);
	private static native long __WaitRelease(long wait);
	private static native boolean __WaitIsRunning(long wait);
	private static native void __WaitRun(long wait);
	private static native void __WaitBreak(long wait);
	private static native void __WaitReset(long wait);
	
	private static native EngineApi __InterfaceQueryEngine();
	
	private static native void __RunOnSystemThread(Runnable callback);
	
	//private static native Activity __InterfaceQueryActivity();
	//private static native ViewGroup __InterfaceQueryContainer();
    private static native double __InterfaceQueryViewScale();
};