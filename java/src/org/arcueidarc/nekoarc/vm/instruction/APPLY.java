package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class APPLY implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.getAcc().apply(vm);
	}

}
