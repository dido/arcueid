package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.True;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class TRUE implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.setAcc(True.T);
	}

}
