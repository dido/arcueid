package org.arcueidarc.nekoarc.types;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

import org.arcueidarc.nekoarc.util.IndexPhantomReference;
import org.arcueidarc.nekoarc.util.LongMap;

public class Fixnum extends Atom
{
	public final long fixnum;
	private static final LongMap<WeakReference<Fixnum>> table = new LongMap<WeakReference<Fixnum>>();
	private static final ReferenceQueue<Fixnum> rq = new ReferenceQueue<Fixnum>();

	static {
		// Thread that removes phantom references to fixnums
		new Thread() {
			public void run()
			{
				for (;;) {
					try {
						@SuppressWarnings("unchecked")
						IndexPhantomReference<Fixnum> ipr = (IndexPhantomReference<Fixnum>) rq.remove();
						table.remove(ipr.index);
					} catch (InterruptedException e) {
					}
				}
			}
		}.start();
	}

	private Fixnum(long x)
	{
		fixnum = x;
	}

	public static Fixnum get(long x)
	{
		Fixnum f;
		if (table.containsKey(x)) {
			WeakReference<Fixnum> wrf = table.get(x);
			f = wrf.get();
			if (f != null)
				return(f);
		}
		f = new Fixnum(x);
		table.put(x, new WeakReference<Fixnum>(f, rq));
		new IndexPhantomReference<Fixnum>(f, rq, x);
		return(f);
	}
}
