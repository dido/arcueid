package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class PUSH implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.push(vm.getAcc());
	}

}
