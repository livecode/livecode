package /*{package}*/ com.runrev.external /*{package}*/

import java.lang.*;

public class LC
{
	public static final USE_CASE_SENSITIVE = 1 << 30;
	public static final USE_CASE_SENSITIVE = 1 << 30;
	public static final USE_CONVERT_OCTALS = 1 << 28;
	public static final USE_NUMBER_FORMAT_DECIMAL = 0 << 26;
	public static final USE_NUMBER_FORMAT_SCIENTIFIC = 1 << 26;
	public static final USE_NUMBER_FORMAT_COMPACT = 2 << 26;

	public class Error : public RuntimeException
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

	public class Array
	{
		private long m_pointer;
		
		public Array()
		{
			m_pointer = Create(0);
		}
		
		public void Release()
		{
			if (m_pointer == 0)
				return;
			
			Release(m_pointer);
			m_pointer = 0;
		}
		
		//////////
		
		public int CountKeys()
		{
			Check();
			return CountKeys(m_pointer, 0, null);
		}
		
		public String[] ListKeys()
		{
			Check();
			return ListKeys(m_pointer, 0, null);
		}
		
		public void RemoveKeys()
		{
			Check();
			return RemoveKeys(m_pointer, 0, null);
		}
		
		//////////
		
		boolean HasKey(String p_key)
		{
			Check();
			return HasKey(m_pointer, 0, null, p_key);
		}
		
		boolean HasExactKey(String p_key)
		{
			Check();
			return HasKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		Object FetchKey(String p_key)
		{
			Check();
			return FetchKey(m_pointer, 0, null, p_key);
		}
		
		Object FetchExactKey(String p_key)
		{
			Check();
			return FetchKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		void StoreKey(String p_key, Object p_value)
		{
			Check();
			StoreKey(m_pointer, 0, null, p_key, p_value);
		}
		
		void StoreExactKey(String p_key, Object p_value)
		{
			Check();
			StoreKey(m_pointer, USE_CASE_SENSITIVE, null, p_key, p_value);
		}
		
		Object RemoveKey(String p_key)
		{
			Check();
			RemoveKey(m_pointer, 0, null, p_key);
		}
		
		Object RemoveExactKey(String p_key)
		{
			Check();
			RemoveKey(m_pointer, USE_CASE_SENSITIVE, null, p_key);
		}
		
		//////////
		
		protected void finalize()
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
	};
	
	public class Object
	{
		private long m_pointer;
		
		public Object(String p_chunk)
		{
			m_pointer = Resolve(p_chunk);
		}
		
		public void Release()
		{
			if (m_pointer == 0)
				return;
				
			if (m_pointer != 0xffffffff)
				Release(m_pointer);
				
			m_pointer = 0;
		}
		
		//////////
		
		public boolean Exists()
		{
			Check();
			if (m_pointer == 0xffffffff)
				return false;
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
	
	private static native long ArrayCreate(int options);
	private static native void ArrayRetain(long array);
	private static native void ArrayRelease(long array);
	private static native int ArrayCountKeys(long array, int options, String[] path);
	private static native String[] ArrayListKeys(long array, int options, String[] path);
	private static native void ArrayRemoveKeys(long array, int options, String[] path);
	
	private static native boolean ArrayHasKey(long array, int options, String[] path, String key);
	private static native Object ArrayFetchKey(long array, int options, String[] path, String key);
	private static native void ArrayStoreKey(long array, int options, String[] path, String key, java.lang.Object value);
	private static native void ArrayRemoveKey(long array, int options, String[] path, String key);
	
	private static native long ObjectResolve(String chunk);
	private static native void ObjectRetain(long object);
	private static native void ObjectRelease(long object);
	private static native boolean ObjectExists(long object);
	private static native void ObjectSend(long object, String message, String signature, java.lang.Object[] arguments);
	private static native void ObjectPost(long object, String message, String signature, java.lang.Object[] arguments);
	private static native java.lang.Object ObjectGet(long object, int options, String property, String key);
	private static native void ObjectSet(long object, int options, String property, String key);
	
	private static native long ContextMe();
	private static native long ContextTarget();
	private static native long ContextDefaultStack();
	private static native long ContextDefaultCard();
	public static native boolean ContextCaseSensitive();
	public static native boolean ContextConvertOctals();
	public static native boolean ContextWholeMatches();
	private static native Object ContextItemDelimiter(int options);
	private static native Object ContextLineDelimiter(int options);
	private static native Object ContextRowDelimiter(int options);
	private static native Object ContextColumnDelimiter(int options);
	private static native Object ContextResult(int options);
	private static native Object ContextIt(int options);
	private static native Object ContextEvaluate(String expression, int options);
	private static native Object ContextExecute(String statements, int options);
	
	private static long WaitCreate(int options);
	private static long WaitRetain(long wait);
	private static long WaitRelease(long wait);
	private static boolean WaitIsRunning(long wait);
	private static void WaitRun(long wait);
	private static void WaitBreak(long wait);
	private static void WaitReset(long wait);
	
	public static native Activity InterfaceQueryActivity();
	public static native ViewGroup InterfaceQueryContainer();
	
	/*{methods}*/
};