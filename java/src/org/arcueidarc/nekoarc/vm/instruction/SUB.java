package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.types.Numeric;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class SUB implements Instruction {

	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		Numeric arg1, arg2;
		arg1 = (Numeric)vm.pop();
		arg2 = (Numeric)vm.getAcc();
		vm.setAcc(arg1.add(arg2.negate()));
	}

}
