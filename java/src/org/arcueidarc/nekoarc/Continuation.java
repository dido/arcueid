package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.types.Vector;

public class Continuation extends Vector
{
	public static final ArcObject TYPE = Symbol.intern("continuation");

	public Continuation(int size)
	{
		super(size);
	}

}
