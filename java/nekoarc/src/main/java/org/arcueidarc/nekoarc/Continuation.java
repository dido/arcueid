package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public interface Continuation
{
	public void restore(VirtualMachine vm, Callable caller);
}
