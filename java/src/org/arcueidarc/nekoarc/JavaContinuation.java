package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class JavaContinuation extends Continuation
{
	private final ArcObject prev;
	private final InvokeThread inv;

	public JavaContinuation(InvokeThread thr, ArcObject pcont)
	{
		super(0);
		prev = pcont;
		inv = thr;
	}

	@Override
	public void restore(VirtualMachine vm)
	{
		// The restoration of a Java continuation should result in the InvokeThread resuming execution while the virtual machine
		// thread waits for it.
		vm.setAcc(inv.sync());
		vm.setCont(prev);
		vm.restorecont();
	}
}
