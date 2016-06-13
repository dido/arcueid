package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class CLS implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int target = (int)vm.instArg() + vm.getIP();
		Closure clos = new Closure(vm.heapenv(), Fixnum.get(target));
		vm.setAcc(clos);
	}

}
