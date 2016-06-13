package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class STE implements Instruction
{

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.setenv(vm.smallInstArg() & 0xff, vm.smallInstArg() & 0xff, vm.getAcc());
	}

}
