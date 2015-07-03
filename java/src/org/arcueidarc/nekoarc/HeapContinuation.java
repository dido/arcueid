package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.types.Vector;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class HeapContinuation extends Vector implements Continuation
{
	public static final ArcObject TYPE = Symbol.intern("continuation");

	public HeapContinuation(int size)
	{
		super(size);
	}


	@Override
	/**
	 * Restore a stack-based continuation.
	 */
	public void restore(VirtualMachine vm, Callable cc)
	{
		Fixnum c = Fixnum.get(this.length());
		for (int i=0; i<this.length(); i++)
			vm.setStackIndex(i, index(i));
		vm.setCont(c);
		vm.restorecont();
	}

	public static HeapContinuation fromStackCont(VirtualMachine vm, ArcObject sc)
	{
		int cc = (int)((Fixnum)sc).fixnum;
		HeapContinuation c = new HeapContinuation(cc);
		for (int i=0; i<cc; i++)
			c.setIndex(i, vm.stackIndex(i));
		return(c);
	}


	@Override
	public int requiredArgs()
	{
		return(1);
	}

	/** The application of a continuation -- this does all the hard work of call/cc */
	@Override
	public ArcObject invoke(InvokeThread thr)
	{
		// XXX -- fill this in
		return null;
	}

}
