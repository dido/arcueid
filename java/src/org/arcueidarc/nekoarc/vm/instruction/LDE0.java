package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class LDE0 implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int indx;

		indx = vm.instArg();
		vm.setAcc(vm.getenv(0, indx));
	}

}
