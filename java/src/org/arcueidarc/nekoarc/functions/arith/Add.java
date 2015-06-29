package org.arcueidarc.nekoarc.functions.arith;

import org.arcueidarc.nekoarc.functions.Builtin;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class Add extends Builtin
{
	public Add()
	{
		super("+", 0, 0, 0, true);
	}

	@Override
	protected ArcObject invoke(VirtualMachine vm)
	{
		ArcObject sum = Fixnum.ZERO;
		for (int i=0; i<vm.argc(); i++)
			sum.add(vm.getenv(0, i));
		return(sum);
	}
	
}
