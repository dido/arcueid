package org.arcueidarc.nekoarc.functions.arith;

import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.functions.Builtin;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;

public class Add extends Builtin
{
	public Add()
	{
		super("+", 0, 0, 0, true);
	}

	@Override
	public ArcObject invoke(InvokeThread vm)
	{
		// XXX - fixme
		return(Fixnum.get(0));
	}
	
}
