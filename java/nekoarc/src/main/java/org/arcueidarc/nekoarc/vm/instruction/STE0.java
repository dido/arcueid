package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class STE0 implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.setenv(0, vm.smallInstArg() & 0xff, vm.getAcc());
	}

}
