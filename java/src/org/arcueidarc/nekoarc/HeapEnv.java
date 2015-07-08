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
	public static ArcObject fromStackEnv(VirtualMachine vm, ArcObject sei)
	{
		if (sei instanceof HeapEnv || sei.is(Nil.NIL))
			return(sei);
		int si = (int)((Fixnum)sei).fixnum;
		int start = (int)((Fixnum)vm.stackIndex(si)).fixnum;
		int size = (int)((Fixnum)vm.stackIndex(si+1)).fixnum;
		ArcObject penv = vm.stackIndex(si+2);
		// Convert previous env into a stack-based one too
		penv = fromStackEnv(vm, penv);
		HeapEnv nenv = new HeapEnv(size, penv);
		// Copy stack env data to new env
		for (int i=0; i<size; i++)
			nenv.setEnv(i, vm.stackIndex(start + i));
		return(nenv);
	}
}
