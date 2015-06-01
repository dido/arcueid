package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class Env
{
	private Env prev;
	private int start, size;

	public Env(Env p, int st, int sz)
	{
		prev = p;
		start = st;
		size = sz;
	}

	public Env prevEnv()
	{
		return(prev);
	}

	public ArcObject getEnv(VirtualMachine vm, int index) throws NekoArcException
	{
		if (index > size)
			throw new NekoArcException("stack environment index exceeded");
		return(vm.stackIndex(start + index));
	}

	public ArcObject setEnv(VirtualMachine vm, int index, ArcObject value)
	{
		if (index > size)
			throw new NekoArcException("stack environment index exceeded");
		return(vm.setStackIndex(start + index, value));
	}
}
