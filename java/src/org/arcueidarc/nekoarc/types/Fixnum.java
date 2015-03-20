package org.arcueidarc.nekoarc.types;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.util.IndexPhantomReference;
import org.arcueidarc.nekoarc.util.LongMap;

public class Fixnum extends Atom
{
	public final ArcObject TYPE = Symbol.intern("fixnum");
	public final long fixnum;
	private static final LongMap<WeakReference<Fixnum>> table = new LongMap<WeakReference<Fixnum>>();
	private static final ReferenceQueue<Fixnum> rq = new ReferenceQueue<Fixnum>();
	public static final Fixnum ZERO = get(0);
	public static final Fixnum ONE = get(1);
	public static final Fixnum TEN = get(10);

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

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	public static Fixnum cast(ArcObject arg, ArcObject caller)
	{
		if (arg instanceof Flonum) {
			return(Fixnum.get((long)((Flonum)arg).flonum));
		} else if (arg instanceof Fixnum) {
			return((Fixnum)arg);
		}
		throw new NekoArcException("Wrong argument type, caller " + caller + " expected a Fixnum, got " + arg);
	}

	@Override
	public ArcObject add(ArcObject ae)
	{
		if (ae instanceof Flonum) {
			Flonum fnum = Flonum.cast(this, this);
			return(fnum.add(ae));
		}
		Fixnum addend = Fixnum.cast(ae, this);
		return(Fixnum.get(this.fixnum + addend.fixnum));
	}

	@Override
	public String toString()
	{
		return(String.valueOf(fixnum));
	}
}
