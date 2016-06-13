package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class HLT implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		vm.halt();
	}

}
