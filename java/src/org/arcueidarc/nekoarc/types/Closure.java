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
		vm.setEnv(newenv);
		// If this is not a call from the vm itself, we need to take the additional step of waking up the VM thread
		// and causing the caller thread to sleep. Unnecessary if this is already a VM thread.
		if (vm != caller) {
			vm.sync().ret(this);				// wakes the VM so it begins executing the closure
			vm.setAcc(caller.sync().retval());	// sleeps the caller until its JavaContinuation is restored
		}
	}
}
