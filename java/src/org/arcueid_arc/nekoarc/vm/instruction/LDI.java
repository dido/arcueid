package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.types.Fixnum;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class LDI implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		long value = vm.instArg();
		vm.setAcc(Fixnum.get(value));
	}
}
