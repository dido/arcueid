package org.arcueidarc.nekoarc.types;

public class Flonum extends Atom
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
}
