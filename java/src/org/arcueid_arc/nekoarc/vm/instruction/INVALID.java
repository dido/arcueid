package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.InvalidInstructionException;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public class INVALID implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws InvalidInstructionException
	{
		throw new InvalidInstructionException();
	}

}
