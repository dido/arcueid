package org.arcueidarc.nekoarc.vm;

import org.arcueidarc.nekoarc.InvalidInstructionException;

public class INVALID implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws InvalidInstructionException
	{
		throw new InvalidInstructionException(vm.getIP());
	}

}
