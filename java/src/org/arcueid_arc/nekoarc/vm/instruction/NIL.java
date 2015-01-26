package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.Nil;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class NIL implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		vm.setAcc(Nil.NIL);
	}
}
