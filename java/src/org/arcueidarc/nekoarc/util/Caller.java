package org.arcueidarc.nekoarc.util;

import java.util.concurrent.SynchronousQueue;

import org.arcueidarc.nekoarc.types.ArcObject;

public class Caller
{
	private final SynchronousQueue<ArcObject> syncqueue;

	public Caller()
	{
		syncqueue = new SynchronousQueue<ArcObject>();
	}

	public void put(ArcObject retval)
	{
		for (;;) {
			try {
				syncqueue.put(retval);
				return;
			} catch (InterruptedException e) { }
		}
	}

	public ArcObject ret()
	{
		for (;;) {
			try {
				return(syncqueue.take());
			} catch (InterruptedException e) { }
		}
	}
}
