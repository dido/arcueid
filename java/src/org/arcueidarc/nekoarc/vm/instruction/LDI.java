package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class LDI implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm)
	{
		long value = vm.instArg();
		vm.setAcc(Fixnum.get(value));
	}
}
