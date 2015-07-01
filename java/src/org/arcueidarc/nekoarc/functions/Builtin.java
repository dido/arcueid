package org.arcueidarc.nekoarc.functions;

import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;

public abstract class Builtin extends ArcObject
{
	public static final Symbol TYPE = (Symbol) Symbol.intern("fn");
	protected final String name;
	protected final int rargs, eargs, oargs;
	protected final boolean variadic;

	protected Builtin(String name, int req, int opt, int extra, boolean va)
	{
		this.name = name;
		rargs = req;
		eargs = extra;
		oargs = opt;
		variadic = va;
	}

	protected Builtin(String name, int req)
	{
		this(name, req, 0, 0, false);
	}

	@Override
	public int requiredArgs()
	{
		return(rargs);
	}

	public int optionalArgs()
	{
		return(oargs);
	}

	public int extraArgs()
	{
		return(eargs);
	}

	public boolean variadicP()
	{
		return(variadic);
	}

	public abstract ArcObject invoke(InvokeThread vm);

	public ArcObject type()
	{
		return(TYPE);
	}
}
