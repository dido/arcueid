package org.arcueid_arc.nekoarc.types;

import java.lang.ref.WeakReference;
import org.arcueid_arc.nekoarc.util.LongMap;

public class Symbol extends Atom
{
	public final String symbol;
	private static final LongMap<WeakReference<Symbol>> symtable = new LongMap<WeakReference<Symbol>>();

	private Symbol(String s)
	{
		symbol = s;
	}

	private static long hash(String s)
	{
		return((long)s.hashCode());
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
		return(sym);
	}
}
