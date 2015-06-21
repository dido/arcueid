package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.types.Vector;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

/** Heap environment */
public class HeapEnv extends Vector
{
	public static final ArcObject TYPE = Symbol.intern("environment");
	private ArcObject prev;

	public HeapEnv(int size, ArcObject p)
	{
		super(size);
		prev = p;
	}

	public ArcObject prevEnv()
	{
		return(prev);
	}

	public ArcObject getEnv(int index) throws NekoArcException
	{
		return(this.index(index));
	}

	public ArcObject setEnv(int index, ArcObject value)
	{
		return(setIndex(index, value));
	}

	// Convert a stack-based environment si into a heap environment. Also affects any
	// environments linked to it.
	public static HeapEnv fromStackEnv(VirtualMachine vm, int si)
	{
		int start = (int)((Fixnum)vm.stackIndex(si)).fixnum;
		int size = (int)((Fixnum)vm.stackIndex(si+1)).fixnum;
		ArcObject penv = vm.stackIndex(si+2);
		// If we have a stack-based env as the previous, convert it into a heap env too
		if (penv instanceof Fixnum)
			penv = fromStackEnv(vm, (int)((Fixnum)penv).fixnum);
		// otherwise, penv must be either NIL or a heap env as well
		HeapEnv nenv = new HeapEnv(size, penv);
		// Copy stack env data to new env
		for (int i=0; i<size; i++)
			nenv.setEnv(i, vm.stackIndex(start + i));
		return(nenv);
	}
}
