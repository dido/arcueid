package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.NekoArcException;

public class Flonum extends Numeric
{
	public static final ArcObject TYPE = Symbol.intern("flonum");
	public double flonum;
	
	public Flonum(double flonum)
	{
		this.flonum = flonum;
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	public static Flonum cast(ArcObject arg, ArcObject caller)
	{
		if (arg instanceof Fixnum) {
			return(new Flonum((double)((Fixnum) arg).fixnum));
		} else if (arg instanceof Flonum) {
			return((Flonum)arg);
		}
		throw new NekoArcException("Wrong argument type, caller " + caller + " expected a Fixnum, got " + arg);
	}

	@Override
	public ArcObject add(ArcObject ae)
	{
		Flonum addend = Flonum.cast(ae, this);
		return(new Flonum(this.flonum + addend.flonum));
	}

	@Override
	public Numeric negate()
	{
		return(new Flonum(-this.flonum));
	}
}
