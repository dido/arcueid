package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Unbound;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class JBND implements Instruction
{

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int target = (int)vm.instArg();
		if (!vm.getAcc().is(Unbound.UNBOUND))
			vm.setIP(vm.getIP() + target);
	}

}
