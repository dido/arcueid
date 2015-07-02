package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class JavaContinuation extends Continuation
{
	private final ArcObject prev;
	private final Callable caller;

	public JavaContinuation(Callable c, ArcObject pcont)
	{
		super(0);
		prev = pcont;
		caller = c;
	}

	@Override
	public void restore(VirtualMachine vm)
	{
		// The restoration of a Java continuation should result in the InvokeThread resuming execution while the virtual machine
		// thread waits for it.
		vm.setAcc(caller.caller().ret());
		vm.setCont(prev);
		vm.restorecont();
	}
}
