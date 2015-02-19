package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;

public class True extends ArcObject
{
	public static final True T = new True();

	private True()
	{
	}

	public ArcObject type()
	{
		return(Symbol.TYPE);
	}
}
