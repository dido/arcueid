package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class MENV implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		vm.menv((int)(vm.smallInstArg() & 0xff));
	}

}
