package org.arcueidarc.nekoarc.util;

import java.util.concurrent.SynchronousQueue;

import org.arcueidarc.nekoarc.types.ArcObject;

public class CallSync
{
	private final SynchronousQueue<ArcObject> syncqueue;

	public CallSync()
	{
		syncqueue = new SynchronousQueue<ArcObject>();
	}

	public void ret(ArcObject retval)
	{
		for (;;) {
			try {
				syncqueue.put(retval);
				return;
			} catch (InterruptedException e) { }
		}
	}

	public ArcObject retval()
	{
		for (;;) {
			try {
				return(syncqueue.take());
			} catch (InterruptedException e) { }
		}
	}
}
