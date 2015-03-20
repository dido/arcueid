package org.arcueidarc.nekoarc.types;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.util.IndexPhantomReference;
import org.arcueidarc.nekoarc.util.LongMap;
import org.arcueidarc.nekoarc.util.MurmurHash;

public class Symbol extends Atom
{
	public final String symbol;
	private static final LongMap<WeakReference<Symbol>> symtable = new LongMap<WeakReference<Symbol>>();
	private static final ReferenceQueue<Symbol> rq = new ReferenceQueue<Symbol>();
	public static final ArcObject TYPE = Symbol.intern("sym");

	static {
		// Thread that removes phantom references to fixnums
		new Thread() {
			public void run()
			{
				for (;;) {
					try {
						@SuppressWarnings("unchecked")
						IndexPhantomReference<Symbol> ipr = (IndexPhantomReference<Symbol>) rq.remove();
						symtable.remove(ipr.index);
					} catch (InterruptedException e) {
					}
				}
			}
		}.start();
	}

	private Symbol(String s)
	{
		symbol = s;
	}

	private static long hash(String s)
	{
//		return((long)s.hashCode());
		return(MurmurHash.hash(s));
	}

	public long hash()
	{
		return(hash(this.symbol));
	}

	public static ArcObject intern(String s)
	{
		Symbol sym;
		long hc = hash(s);

		if (s.equals("t"))
			return(True.T);
		if (s.equals("nil"))
			return(Nil.NIL);

		if (symtable.containsKey(hc)) {
			WeakReference<Symbol> wref = symtable.get(hc);
			sym = wref.get();
			if (sym != null)
				return(sym);
		}
		sym = new Symbol(s);
		symtable.put(hc, new WeakReference<Symbol>(sym));
		new IndexPhantomReference<Symbol>(sym, rq, hc);
		return(sym);
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	@Override
	public String toString()
	{
		return(this.symbol);
	}
}
