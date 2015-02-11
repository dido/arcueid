package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class LDL implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int offset = vm.instArg();
		vm.setAcc(vm.literal(offset));
	}

}
