package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class LDL implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int offset = vm.instArg();
		vm.setAcc(vm.literal(offset));
	}

}
