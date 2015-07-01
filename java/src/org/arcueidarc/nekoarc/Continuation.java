package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;
import org.arcueidarc.nekoarc.types.Symbol;
import org.arcueidarc.nekoarc.types.Vector;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public class Continuation extends Vector
{
	public static final ArcObject TYPE = Symbol.intern("continuation");

	public Continuation(int size)
	{
		super(size);
	}


	/**
	 * Restore a stack-based continuation.
	 */
	public void restore(VirtualMachine vm)
	{
		Fixnum c = Fixnum.get(this.length());
		for (int i=0; i<this.length(); i++)
			vm.setStackIndex(i, index(i));
		vm.setCont(c);
		vm.restorecont();
	}

	public static Continuation fromStackCont(VirtualMachine vm, ArcObject sc)
	{
		int cc = (int)((Fixnum)sc).fixnum;
		Continuation c = new Continuation(cc);
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
