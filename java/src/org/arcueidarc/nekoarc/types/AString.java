package org.arcueidarc.nekoarc.types;

public class AString extends ArcObject
{
	public static final ArcObject TYPE = Symbol.intern("string");
	public final String string;

	public AString(String str)
	{
		this.string = str;
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	@Override
	public ArcObject add(ArcObject ae)
	{
		String as = ae.toString();
		StringBuilder sb = new StringBuilder(this.string);
		sb.append(as);
		return(new AString(sb.toString()));
	}

	@Override
	public String toString()
	{
		return(string);
	}
}
