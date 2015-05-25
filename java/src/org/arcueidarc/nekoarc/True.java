package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;

public class True extends ArcObject
{
	public static final ArcObject TYPE = Symbol.TYPE;
	public static final True T = new True("t");
	private String rep;

	private True(String rep)
	{
		this.rep = rep;
	}
	
	public ArcObject type()
	{
		return(Symbol.TYPE);
	}

	public String toString()
	{
		return(rep);
	}
}
