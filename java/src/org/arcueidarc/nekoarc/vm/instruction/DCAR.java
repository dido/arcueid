package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

// just like CAR, except that it works on nil and unbound, producing unbound.
public class DCAR implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		if (vm.getAcc().is(Nil.NIL) || vm.getAcc().is(Unbound.UNBOUND))
			vm.setAcc(Unbound.UNBOUND);
		else
			vm.setAcc(vm.getAcc().car());
	}
}
