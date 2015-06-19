package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class Closure extends Cons
{
	public static final ArcObject TYPE = Symbol.intern("closure");

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
