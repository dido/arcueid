package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class POP implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.setAcc(vm.pop());
	}

}
