package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.Instruction;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class ENV implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int minenv, dsenv, optenv;
		minenv = vm.smallInstArg() & 0xff;
		dsenv = vm.smallInstArg() & 0xff;
		optenv = vm.smallInstArg() & 0xff;
		vm.argcheck(minenv, minenv + optenv);
		vm.mkenv(vm.argc(), minenv + optenv - vm.argc() + dsenv);
	}
}
