package org.arcueidarc.nekoarc.functions.arith;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.functions.Builtin;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.types.Fixnum;

public class Add extends Builtin
{
	public Add()
	{
		super("+");
	}
	
	@Override
	protected ArcObject invoke(Cons args)
	{
		if (args instanceof Nil)
			return(Fixnum.ZERO);
		ArcObject result = args.car();
		ArcObject rest = args.cdr();
		while (!(rest instanceof Nil)) {
			result = result.add(rest.car());
			rest = rest.cdr();
		}
		return(result);
	}
	
}
