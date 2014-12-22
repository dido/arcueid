package org.arcueid_arc.nekoarc.types;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.WeakReference;

import org.arcueid_arc.nekoarc.util.IndexPhantomReference;
import org.arcueid_arc.nekoarc.util.LongMap;
import org.arcueid_arc.nekoarc.util.MurmurHash;

public class Symbol extends Atom
{
	public final String symbol;
	private static final LongMap<WeakReference<Symbol>> symtable = new LongMap<WeakReference<Symbol>>();
	private static final ReferenceQueue<Symbol> rq = new ReferenceQueue<Symbol>();

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

	public static Symbol intern(String s)
	{
		Symbol sym;
		long hc = hash(s);

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
}
