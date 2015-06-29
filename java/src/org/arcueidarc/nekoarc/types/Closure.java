package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class Closure extends Cons
{
	public static final ArcObject TYPE = Symbol.intern("closure");

	public Closure(ArcObject ca, ArcObject cd)
	{
		super(ca, cd);
	}

	/** This is the only place where apply should be overridden */
	@Override
	public void apply(VirtualMachine vm)
	{
		ArcObject newenv, newip;
		newenv = this.car();
		newip = this.cdr();
		vm.setIP((int)((Fixnum)newip).fixnum);
		vm.setEnv(newenv);
	}
}
