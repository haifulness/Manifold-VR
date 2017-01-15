package org.geometrygames.geometrygamesshared;


import java.util.concurrent.locks.ReentrantLock;


public class GeometryGamesModel
{
	//	The main thread and the GLSurfaceView thread both need 
	//	to access the ModelData, so use a Lock to prevent conflicts.
	//
	//	Only the lockModelData() method may directly access
	//	the instance variable itsModelDataPRIVATE.
	//	All other code should instead check out a pointer, use it, 
	//	and then check it back in again as soon as possible:
	//
	//		long	md = 0;
	//
	//		md = theModel.lockModelData();
	//		Pass md to the C code to use as desired.
	//		md = theModel.unlockModelData();
	//
	//	or to protect against Java exceptions
	//
	//		long	md = 0;
	//
	//		md = theModel.lockModelData();
	//		try
	//		{
	//			Pass md to the C code to use as desired.
	//		}
	//		finally
	//		{
	//			md = theModel.unlockModelData();
	//		}
	//
	//
	private long			itsModelDataPRIVATE;	//	C pointer to app-specific ModelData; always 64-bit
	private ReentrantLock	itsModelDataLock;

	public GeometryGamesModel()
	{
		super();
		
		//	Set up an app-specific model.
		itsModelDataPRIVATE = GeometryGamesJNIWrapper.alloc_model_data();
		GeometryGamesJNIWrapper.set_up_model_data(itsModelDataPRIVATE);

		//	Set up a lock.
		itsModelDataLock = new ReentrantLock();
	}
	
	@Override
	public void finalize() throws Throwable
	{
		//	Note:  Releasing memory allocated in C code is tricky in Java.
		//	Various articles online warn that there's no way
		//	to know when -- or even if -- finalize() will get called.
		//	We could write a custom method to release itsModelDataPRIVATE,
		//	but we'd need to manually keep track of when this ModelData
		//	was eligible to be finalized.
		
		//	Shut down the app-specific model.
		GeometryGamesJNIWrapper.shut_down_model_data(itsModelDataPRIVATE);
		itsModelDataPRIVATE = GeometryGamesJNIWrapper.free_model_data(itsModelDataPRIVATE);

		//	The Java runtime will automatically release the lock.

		super.finalize();
	}
	
	public long lockModelData()
	{
		//	lock() blocks until the lock becomes available.
		itsModelDataLock.lock();

		//	Return the ModelData pointer to the caller as a long int.
		return itsModelDataPRIVATE;
	}
	
	public long unlockModelData()
	{
		//	Unlock the lock.
		itsModelDataLock.unlock();
		
		//	Return 0 to the caller as a convenience value,
		//	so the caller may zero out its copy of the Model Data "pointer".
		return 0;
	}
}
