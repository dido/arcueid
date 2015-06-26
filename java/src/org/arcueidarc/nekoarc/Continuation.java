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
	public Fixnum restore(VirtualMachine vm)
	{
		Fixnum c = Fixnum.get(this.length());
		for (int i=0; i<this.length(); i++)
			vm.setStackIndex(i, index(i));
		return(c);
	}

	public static Continuation fromStackCont(VirtualMachine vm, Fixnum sc)
	{
		Continuation c = new Continuation((int)sc.fixnum);
		for (int i=0; i<sc.fixnum; i++)
			c.setIndex(i, vm.stackIndex(i));
		return(c);
	}
}
