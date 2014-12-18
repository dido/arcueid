package org.arcueid_arc.nekoarc.util;

import java.lang.ref.PhantomReference;
import java.lang.ref.ReferenceQueue;

public class IndexPhantomReference<T> extends PhantomReference<T>
{
	public final long index;

	public IndexPhantomReference(T referent, ReferenceQueue<? super T> q, long index)
	{
		super(referent, q);
		this.index = index;
	}
}
