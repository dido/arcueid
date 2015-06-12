package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Cons;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class ENVR implements Instruction
{
	@Override
	public void invoke(VirtualMachine vm) throws NekoArcException
	{
		int minenv, dsenv, optenv, i;
		minenv = vm.smallInstArg() & 0xff;
		dsenv = vm.smallInstArg() & 0xff;
		optenv = vm.smallInstArg() & 0xff;
		if (vm.argc() < minenv)
			throw new NekoArcException("too few arguments, at least " + minenv + " required, " + vm.argc() + " passed");
		ArcObject rest = Nil.NIL;
		/* Swallow as many extra arguments as are available into the rest parameter,
		 * up to the minimum + optional number of arguments. */
		for (i = vm.argc(); i>(minenv + optenv); i--)
			rest = new Cons(vm.pop(), rest);
		vm.mkenv(i, minenv + optenv - i + dsenv + 1);
		/* store the rest parameter */
		vm.setenv(0, minenv +optenv + dsenv, rest);
	}

}
