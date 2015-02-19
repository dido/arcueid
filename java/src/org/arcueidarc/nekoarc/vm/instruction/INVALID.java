package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.InvalidInstructionException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class INVALID implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws InvalidInstructionException
	{
		throw new InvalidInstructionException(vm.getIP());
	}

}
