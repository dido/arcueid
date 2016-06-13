package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.util.Callable;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

/** The main reason this class exists is that Java is a weak-sauce language that doesn't have closures or true continuations. We have to
 *  emulate them using threads. */
public class InvokeThread extends Thread
{
	public final Callable caller;
	public final ArcObject obj;
	public final VirtualMachine vm;

	public InvokeThread(VirtualMachine v, Callable c, ArcObject o)
	{
		vm = v;
		caller = c;
		obj = o;
	}

	@Override
	public void run()
	{
		// Perform our function's thing
		ArcObject ret = obj.invoke(this);
		// Restore the continuation created by the caller
		vm.restorecont(caller);
		// Return the result to our caller's thread, waking them up
		caller.sync().ret(ret);
		// and this invoke thread's work is ended
	}

	public ArcObject getenv(int i, int j)
	{
		return(vm.getenv(i, j));
	}

	/** Perform a call to some Arc object. This should prolly work for ANY ArcObject that has a proper invoke method defined.  If it is a built-in or
	 *  some function defined in Java, that function will run in its own thread while the current object's thread is suspended. */
	public ArcObject apply(ArcObject fn, ArcObject...args)
	{
		// First, push all of the arguments to the stack
		for (ArcObject arg : args)
			vm.push(arg);

		// new continuation
		vm.setCont(new JavaContinuation(vm, obj));

		// Apply the function.
		fn.apply(vm, obj);

		return(vm.getAcc());
	}
}
