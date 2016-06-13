package org.arcueidarc.nekoarc.types;

public class Vector extends ArcObject
{
	public static final ArcObject TYPE = Symbol.intern("vector");
	private ArcObject[] vec;

	public Vector(int length)
	{
		vec = new ArcObject[length];
	}

	public ArcObject index(int i)
	{
		return(vec[i]);
	}

	public ArcObject setIndex(int i, ArcObject val)
	{
		return(vec[i] = val);
	}

	public int length()
	{
		return(vec.length);
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	@Override
	public String toString()
	{
		return("<" + type().toString() + ">");
	}

}
