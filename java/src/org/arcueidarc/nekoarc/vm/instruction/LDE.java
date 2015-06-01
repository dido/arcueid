package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class LDE implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int env, indx;
		env = vm.instArg();
		indx = vm.instArg();
		vm.setAcc(vm.getenv(env, indx));
	}

}
