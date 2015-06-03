package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
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
		if (vm.argc() < minenv)
			throw new NekoArcException("too few arguments, at least " + minenv + " required, " + vm.argc() + " passed");
		if (vm.argc() > minenv + optenv)
			throw new NekoArcException("too many arguments, at most " + (minenv + optenv) + " allowed, " + vm.argc() + " passed");
		vm.mkenv(vm.argc(), minenv + optenv - vm.argc() + dsenv);
	}
}
