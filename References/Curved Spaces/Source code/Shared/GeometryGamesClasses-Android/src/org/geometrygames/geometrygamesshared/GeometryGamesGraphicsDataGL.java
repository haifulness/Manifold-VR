package org.geometrygames.geometrygamesshared;


import java.util.concurrent.locks.ReentrantLock;


public class GeometryGamesGraphicsDataGL
{
	//	The main thread and the GLSurfaceView thread both need 
	//	to access the GraphicsDataGL, so use a Lock to prevent conflicts.
	//
	//	Only the lockGraphicsData() method may directly access
	//	the instance variable itsGraphicsDataPRIVATE.
	//	All other code should instead check out a pointer, use it, 
	//	and then check it back in again as soon as possible:
	//
	//		long	gd = 0;
	//
	//		gd = theGraphicsDataGL.lockGraphicsData();
	//		Pass gd to the C code to use as desired.
	//		gd = theGraphicsDataGL.unlockGraphicsData();
	//
	//	or to protect against Java exceptions
	//
	//		long	gd = 0;
	//
	//		gd = theGraphicsDataGL.lockGraphicsData();
	//		try
	//		{
	//			Pass gd to the C code to use as desired.
	//		}
	//		finally
	//		{
	//			gd = theGraphicsDataGL.unlockGraphicsData();
	//		}
	//
	//
	private long			itsGraphicsDataPRIVATE;	//	C pointer to app-specific GraphicsDataGL; always 64-bit
	private ReentrantLock	itsGraphicsDataLock;

	public GeometryGamesGraphicsDataGL()
	{
		super();
		
		//	Set up an app-specific GraphicsDataGL.
		itsGraphicsDataPRIVATE = GeometryGamesJNIWrapper.alloc_graphics_data_gl();
		GeometryGamesJNIWrapper.zero_graphics_data_gl(itsGraphicsDataPRIVATE);

		//	Set up a lock.
		itsGraphicsDataLock = new ReentrantLock();
	}
	
	@Override
	public void finalize() throws Throwable
	{
		//	Note:  Releasing memory allocated in C code is tricky in Java.
		//	Various articles online warn that there's no way
		//	to know when -- or even if -- finalize() will get called.
		//	We could write a custom method to release itsGraphicsDataPRIVATE,
		//	but we'd need to manually keep track of when this GraphicsDataGL
		//	was eligible to be finalized.
		
		//	Free the app-specific GraphicsDataGL.
		itsGraphicsDataPRIVATE = GeometryGamesJNIWrapper.free_graphics_data_gl(itsGraphicsDataPRIVATE);

		//	The Java runtime will automatically release the lock.

		super.finalize();
	}
	
	public long lockGraphicsData()
	{
		//	lock() blocks until the lock becomes available.
		itsGraphicsDataLock.lock();

		//	Return the GraphicsDataGL pointer to the caller as a long int.
		return itsGraphicsDataPRIVATE;
	}
	
	public long unlockGraphicsData()
	{
		//	Unlock the lock.
		itsGraphicsDataLock.unlock();
		
		//	Return 0 to the caller as a convenience value,
		//	so the caller may zero out its copy of the GraphicsDataGL "pointer".
		return 0;
	}
}
