package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class JF implements Instruction
{

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int target = (int)vm.instArg();
		if (vm.getAcc() instanceof Nil)
			vm.setIP(vm.getIP() + target);
	}

}
