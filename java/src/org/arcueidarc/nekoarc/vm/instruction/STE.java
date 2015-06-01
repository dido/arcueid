package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class STE implements Instruction
{

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int env, indx;
		env = vm.instArg();
		indx = vm.instArg();
		vm.setenv(env, indx, vm.getAcc());
	}

}
