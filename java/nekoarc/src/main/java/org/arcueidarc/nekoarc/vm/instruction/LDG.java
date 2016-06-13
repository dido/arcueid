package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class LDG implements Instruction
{

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int offset = vm.instArg();
		Symbol sym = (Symbol)vm.literal(offset);
		vm.setAcc(vm.value(sym));
	}

}
