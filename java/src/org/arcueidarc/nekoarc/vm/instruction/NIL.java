package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class NIL implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		vm.setAcc(Nil.NIL);
	}
}
