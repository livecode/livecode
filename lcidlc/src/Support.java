package [[ExternalPrefix]]

class LCArray

public class LC
{
	public class Array
	{
		private long m_pointer;
		
		public Array()
		{
			m_pointer = Create(0);
		}
		
		public void release()
		{
			if (m_pointer == 0)
				return;
			
			Release(m_pointer);
		}
		
		protected void finalize() throws Throwable
		{
			release();
			super.finalize();
		}
		
		private static native long Create(int options);
		private static native void Retain(long array);
		private static native void Release(long array);
		private static native int CountKeys(long array, int options, String[] path);
		private static native String[] ListAllKeys(long array, int options, String[] path);
		private static native void RemoveAllKeys(long array, int options, String[] path);
		
		private static native bool HasKey(long array, int options, String[] path, String key);
		private static native Object FetchKey(long array, int options, String[] path, String key);
		private static native void StoreKey(long array, int options, String[] path, String key, Object value);
		private static native void RemoveKey(long array, int options, String[] path, String key);
		
		
	};
	
	public static native Activity InterfaceQueryActivity();
	public static native ViewGroup InterfaceQueryContainer();
	
	[[ExternalMethods]]
};