package org.arcueidarc.nekoarc.types;

public class Cons extends ArcObject
{
	public static final ArcObject TYPE = Symbol.intern("cons");
	private ArcObject car;
	private ArcObject cdr;

	public Cons()
	{
	}

	public Cons(ArcObject car, ArcObject cdr)
	{
		this.car = car;
		this.cdr = cdr;
	}

	@Override
	public ArcObject type()
	{
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public ArcObject car()
	{
		return(car);
	}

	@Override
	public ArcObject cdr()
	{
		return(cdr);
	}
}

