package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class HLT implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		vm.halt();
	}

}
