package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class DCDR implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		if (vm.getAcc().is(Nil.NIL) || vm.getAcc().is(Unbound.UNBOUND))
			vm.setAcc(Unbound.UNBOUND);
		else
			vm.setAcc(vm.getAcc().cdr());
	}

}
