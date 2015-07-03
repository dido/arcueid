package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.util.Callable;
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
	public void apply(VirtualMachine vm, Callable caller)
	{
		ArcObject newenv, newip;
		newenv = this.car();
		newip = this.cdr();
		vm.setIP((int)((Fixnum)newip).fixnum);
		vm.setenvreg(newenv);
		// If this is not a call from the vm itself, some other additional actions need to be taken.
		// 1. The virtual machine thread should be resumed.
		// 2. The caller must be suspended until the continuation it created is restored.
		if (vm != caller) {
			vm.sync().ret(this);				// wakes the VM so it begins executing the closure
			vm.setAcc(caller.sync().retval());	// sleeps the caller until its own continuation is restored
		}
	}
}
