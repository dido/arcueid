package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.util.CallSync;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public abstract class ArcObject implements Callable
{
	private final CallSync caller = new CallSync();

	public ArcObject car()
	{
		throw new NekoArcException("Can't take car of " + this.type());
	}

	public ArcObject cdr()
	{
		throw new NekoArcException("Can't take cdr of " + this.type());
	}

	public ArcObject scar(ArcObject ncar)
	{
		throw new NekoArcException("Can't set car of " + this.type());
	}

	public ArcObject scdr(ArcObject ncar)
	{
		throw new NekoArcException("Can't set cdr of " + this.type());
	}

	
	public ArcObject sref(ArcObject value, ArcObject index)
	{
		throw new NekoArcException("Can't sref" + this + "( a " + this.type() + "), other args were " + value + ", " + index);
	}

	public ArcObject add(ArcObject other)
	{
		throw new NekoArcException("add not implemented for " + this.type() + " " + this);
	}

	public long len()
	{
		throw new NekoArcException("len: expects one string, list, or hash argument");
	}

	public abstract ArcObject type();

	public int requiredArgs()
	{
		throw new NekoArcException("Cannot invoke object of type " + type());
	}

	public int optionalArgs()
	{
		return(0);
	}

	public int extraArgs()
	{
		return(0);
	}

	public boolean variadicP()
	{
		return(false);
	}

	/** The basic apply. This should normally not be overridden. Only Closure should probably override it because it runs completely within the vm. */
	public void apply(VirtualMachine vm, Callable caller)
	{
		int minenv, dsenv, optenv;
		minenv = requiredArgs();
		dsenv = extraArgs();
		optenv = optionalArgs();
		if (variadicP()) {
			int i;
			vm.argcheck(minenv, -1);
			ArcObject rest = Nil.NIL;
			for (i = vm.argc(); i>(minenv + optenv); i--)
				rest = new Cons(vm.pop(), rest);
			vm.mkenv(i, minenv + optenv - i + dsenv + 1);
			/* store the rest parameter */
			vm.setenv(0, minenv + optenv + dsenv, rest);
		} else {
			vm.argcheck(minenv, minenv + optenv);
			vm.mkenv(vm.argc(), minenv + optenv - vm.argc() + dsenv);
		}

		// Start the invoke thread
		InvokeThread thr = new InvokeThread(vm, caller, this);
		new Thread(thr).start();

		// Suspend the caller's thread until the invoke thread returns
		vm.setAcc(caller.sync().retval());
		vm.restorecont();
	}

	public ArcObject invoke(InvokeThread vm)
	{
		throw new NekoArcException("Cannot invoke object of type " + type());
	}

	public String toString()
	{
		throw new NekoArcException("Type " + type() + " has no string representation");
	}

	// default implementation
	public boolean is(ArcObject other)
	{
		return(this == other);
	}

	@Override
	public CallSync sync()
	{
		return(caller );
	}
}
