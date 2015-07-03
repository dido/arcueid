package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class JavaContinuation extends ArcObject implements Continuation
{
	public static final ArcObject TYPE = Symbol.intern("continuation");

	private final ArcObject prev;
	private final Callable caller;
	private final ArcObject env;

	public JavaContinuation(VirtualMachine vm, Callable c)
	{
		prev = vm.getCont();
		caller = c;
		env = vm.heapenv();		// copy current env to heap
	}

	@Override
	public void restore(VirtualMachine vm, Callable cc)
	{
		vm.setCont(prev);
		vm.setenvreg(env);
		// This will re-enable the thread represented by this continuation
		// This will stop the thread which restored this continuation
		if (vm == cc) {
			caller.sync().ret(vm.getAcc());
			vm.setAcc(cc.sync().retval());
		}
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}
}
