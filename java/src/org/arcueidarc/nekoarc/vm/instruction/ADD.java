package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class ADD implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		ArcObject arg1, arg2;
		arg1 = vm.pop();
		arg2 = vm.getAcc();
		vm.setAcc(arg1.add(arg2));
	}

}
