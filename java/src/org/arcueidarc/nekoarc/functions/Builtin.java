package org.arcueidarc.nekoarc.functions;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public abstract class Builtin extends ArcObject
{
	public static final Symbol TYPE = (Symbol) Symbol.intern("fn");
	protected final String name;

	protected Builtin(String name)
	{
		this.name = name;
	}

	abstract protected ArcObject invoke(Cons args);

	public void invoke(VirtualMachine vm, Cons args)
	{
		vm.setAcc(invoke(args));
	}

	public ArcObject type()
	{
		return(TYPE);
	}
}
