package org.arcueidarc.nekoarc.functions;

import org.arcueidarc.nekoarc.Continuation;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class CCC extends Builtin
{
	protected CCC()
	{
		super("ccc", 1);
	}

	@Override
	protected ArcObject invoke(VirtualMachine vm)
	{
		ArcObject cont = vm.getCont();
		if (cont instanceof Continuation)
			return(cont);
		// Convert the stack continuation into a heap continuation and apply it to the closure arg
		cont = Continuation.fromStackCont(vm, cont);
		vm.setargc(1);
		vm.push(cont);
		vm.getenv(0, 0).apply(vm);
		// We should not get here if the passed argument was a closure. If it was another built-in, we might get here.
		return(vm.getAcc());
	}

}
